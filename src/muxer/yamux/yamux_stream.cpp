/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux_stream.hpp>

#include <cassert>

#include <libp2p/muxer/yamux/yamux_frame.hpp>

#define TRACE_ENABLED 0
#include <libp2p/common/trace.hpp>

namespace libp2p::connection {

  namespace {
    auto log() {
      static auto logger = log::createLogger("yx-stream");
      return logger.get();
    }
  }  // namespace

  YamuxStream::YamuxStream(
      std::shared_ptr<connection::SecureConnection> connection,
      YamuxStreamFeedback &feedback, uint32_t stream_id,
      size_t maximum_window_size, size_t write_queue_limit)
      : connection_(std::move(connection)),
        feedback_(feedback),
        stream_id_(stream_id),
        window_size_(YamuxFrame::kInitialWindowSize),
        peers_window_size_(YamuxFrame::kInitialWindowSize),
        maximum_window_size_(maximum_window_size),
        write_queue_(write_queue_limit) {
    assert(connection_);
    assert(stream_id_ > 0);
    assert(window_size_ <= maximum_window_size_);
    assert(peers_window_size_ <= maximum_window_size_);
    assert(write_queue_limit >= maximum_window_size_);
  }

  void YamuxStream::read(gsl::span<uint8_t> out, size_t bytes,
                         ReadCallbackFunc cb) {
    doRead(out, bytes, std::move(cb), false);
  }

  void YamuxStream::readSome(gsl::span<uint8_t> out, size_t bytes,
                             ReadCallbackFunc cb) {
    doRead(out, bytes, std::move(cb), true);
  }

  void YamuxStream::deferReadCallback(outcome::result<size_t> res,
                                      ReadCallbackFunc cb) {
    if (no_more_callbacks_) {
      log()->debug("{} closed by client, ignoring callback", stream_id_);
      return;
    }
    feedback_.deferCall([wptr = weak_from_this(), res, cb = std::move(cb)]() {
      auto self = wptr.lock();
      if (self && !self->no_more_callbacks_) {
        cb(res);
      }
    });
  }

  void YamuxStream::write(gsl::span<const uint8_t> in, size_t bytes,
                          WriteCallbackFunc cb) {
    doWrite(in, bytes, std::move(cb), false);
  }

  void YamuxStream::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                              WriteCallbackFunc cb) {
    doWrite(in, bytes, std::move(cb), true);
  }

  void YamuxStream::deferWriteCallback(std::error_code ec,
                                       WriteCallbackFunc cb) {
    if (no_more_callbacks_) {
      log()->debug("{} closed by client, ignoring callback", stream_id_);
      return;
    }
    feedback_.deferCall([wptr = weak_from_this(), ec, cb = std::move(cb)]() {
      auto self = wptr.lock();
      if (self && !self->no_more_callbacks_) {
        cb(ec);
      }
    });
  }

  bool YamuxStream::isClosed() const noexcept {
    return close_reason_.value() != 0;
  }

  void YamuxStream::close(VoidResultHandlerFunc cb) {
    if (isClosed()) {
      if (cb) {
        feedback_.deferCall([wptr{weak_from_this()}, cb{std::move(cb)}] {
          auto self = wptr.lock();
          if (self) {
            cb(self->close_reason_);
          }
        });
      }
      return;
    }

    close_cb_ = std::move(cb);

    if (!isClosedForWrite()) {
      // closing for writes
      is_writable_ = false;

      // sends FIN after data is sent
      doWrite();
    }
  }

  std::pair<YamuxStream::VoidResultHandlerFunc, outcome::result<void>>
  YamuxStream::closeCompleted() {
    std::pair<VoidResultHandlerFunc, outcome::result<void>> p{
        VoidResultHandlerFunc{}, outcome::success()};
    if (!close_reason_) {
      close_reason_ = Error::STREAM_CLOSED_BY_HOST;
    } else if (close_reason_ != Error::STREAM_CLOSED_BY_HOST) {
      p.second = close_reason_;
    }
    if (close_cb_) {
      p.first.swap(close_cb_);
    }
    return p;
  }

  bool YamuxStream::isClosedForRead() const noexcept {
    return !is_readable_;
  }

  bool YamuxStream::isClosedForWrite() const noexcept {
    return !is_writable_;
  }

  void YamuxStream::reset() {
    no_more_callbacks_ = true;
    feedback_.resetStream(stream_id_);
    doClose(Error::STREAM_RESET_BY_HOST, true);
  }

  void YamuxStream::adjustWindowSize(uint32_t new_size,
                                     VoidResultHandlerFunc cb) {
    std::error_code ec = close_reason_;
    if (!ec) {
      if (!is_readable_) {
        ec = Error::STREAM_NOT_READABLE;
      } else if (new_size > maximum_window_size_
                 || new_size < peers_window_size_) {
        ec = Error::STREAM_INVALID_WINDOW_SIZE;
      }
    }

    if (!ec && new_size > peers_window_size_) {
      // Doing this optimistic way, if other side don't like the window update
      // then it would RST

      feedback_.ackReceivedBytes(stream_id_, new_size - peers_window_size_);
      peers_window_size_ = new_size;
    }

    if (cb) {
      feedback_.deferCall([wptr = weak_from_this(), cb = std::move(cb), ec]() {
        auto self = wptr.lock();
        if (!self) {
          return;
        }
        if (self->no_more_callbacks_) {
          return;
        }
        if (!ec) {
          cb(outcome::success());
        } else {
          cb(ec);
        }
      });
    }
  }

  outcome::result<peer::PeerId> YamuxStream::remotePeerId() const {
    return connection_->remotePeer();
  }

  outcome::result<bool> YamuxStream::isInitiator() const {
    return connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> YamuxStream::localMultiaddr() const {
    return connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> YamuxStream::remoteMultiaddr() const {
    return connection_->remoteMultiaddr();
  }

  void YamuxStream::increaseSendWindow(size_t delta) {
    if (delta > 0) {
      window_size_ += delta;
      TRACE("stream {} send window increased by {} to {}", stream_id_, delta,
            window_size_);
      doWrite();
    }
  }

  YamuxStream::DataFromConnectionResult YamuxStream::onDataReceived(
      gsl::span<uint8_t> bytes) {
    auto sz = static_cast<size_t>(bytes.size());

    if (sz == 0) {
      log()->critical("zero data packet received - should not get here");
      return kKeepStream;
    }

    TRACE("stream {} read {} bytes", stream_id_, sz);
    if (sz < 80) {
      TRACE("{}", common::dumpBin(bytes));
    }

    bool overflow = false;
    bool read_completed = false;
    size_t bytes_consumed = 0;
    std::pair<ReadCallbackFunc, outcome::result<size_t>> read_cb_and_res{
        ReadCallbackFunc{}, 0};

    // First transfer bytes to client if available
    if (is_reading_) {
      [[maybe_unused]] auto bytes_needed =
          static_cast<size_t>(external_read_buffer_.size());

      assert(bytes_needed > 0);
      assert(internal_read_buffer_.empty());

      // if sz > bytes_needed then internal buffer will be non empty after
      // this
      bytes_consumed =
          internal_read_buffer_.addAndConsume(bytes, external_read_buffer_);

      assert(bytes_consumed > 0);

      external_read_buffer_ =
          external_read_buffer_.subspan(static_cast<ptrdiff_t>(bytes_consumed));

      read_completed = external_read_buffer_.empty();
      if (reading_some_) {
        read_message_size_ = bytes_consumed;
        read_completed = true;
      }

      if (read_completed) {
        read_cb_and_res = readCompleted();
      } else {
        assert(bytes_consumed < bytes_needed);
      }
    } else {
      internal_read_buffer_.add(bytes);
    }

    if (!internal_read_buffer_.empty()) {
      overflow = (internal_read_buffer_.size() > peers_window_size_);
      if (overflow) {
        log()->debug("read buffer overflow {} > {}, stream {}",
                     internal_read_buffer_.size(), peers_window_size_,
                     stream_id_);
      } else {
        TRACE("stream {} receive window reduced by {} to {}", stream_id_,
              internal_read_buffer_.size(),
              peers_window_size_ - internal_read_buffer_.size());
      }
    }

    if (isClosed()) {
      // already closed, maybe error
      return kRemoveStreamAndSendRst;
    }

    if (overflow) {
      doClose(Error::STREAM_RECEIVE_OVERFLOW, false);
    } else if (bytes_consumed > 0) {
      feedback_.ackReceivedBytes(stream_id_, bytes_consumed);
      TRACE("stream {} receive window increased by {} to {}", stream_id_,
            bytes_consumed, peers_window_size_ - internal_read_buffer_.size());
    }

    if (read_cb_and_res.first) {
      read_cb_and_res.first(read_cb_and_res.second);
    }
    return overflow ? kRemoveStreamAndSendRst : kKeepStream;
  }

  YamuxStream::DataFromConnectionResult YamuxStream::onFINReceived() {
    if (isClosed()) {
      // already closed, maybe error
      return kRemoveStreamAndSendRst;
    }

    is_readable_ = false;

    if (!is_writable_) {
      doClose(Error::STREAM_CLOSED_BY_HOST, true);

      // connection will remove stream
      return kRemoveStream;
    }

    if (is_reading_) {
      // Half closed, client may still write and FIN

      auto cb_and_result = readCompleted();
      if (cb_and_result.first) {
        cb_and_result.first(cb_and_result.second);
      }
    }

    return kKeepStream;
  }

  void YamuxStream::onRSTReceived() {
    if (isClosed()) {
      // already closed, maybe error
      return;
    }

    doClose(Error::STREAM_RESET_BY_PEER, true);
  }

  void YamuxStream::onDataWritten(size_t bytes) {
    auto result = write_queue_.ackDataSent(bytes);
    if (!result.data_consistent) {
      log()->error("write queue ack failed, stream {}", stream_id_);
      feedback_.resetStream(stream_id_);
      doClose(Error::STREAM_INTERNAL_ERROR, true);
      return;
    }

    if (result.cb && !no_more_callbacks_) {
      result.cb(result.size_to_ack);
    }
  }

  void YamuxStream::closedByConnection(std::error_code ec) {
    doClose(ec, true);
  }

  void YamuxStream::doClose(std::error_code ec, bool notify_read_side) {
    assert(ec);

    if (close_reason_) {
      // already closed
      return;
    }

    close_reason_ = ec;
    is_readable_ = false;
    is_writable_ = false;

    std::pair<ReadCallbackFunc, outcome::result<size_t>> read_cb_and_res{
        ReadCallbackFunc{}, 0};

    if (notify_read_side && is_reading_) {
      read_cb_and_res = readCompleted();
    }

    internal_read_buffer_.clear();

    auto write_callbacks = write_queue_.getAllCallbacks();

    write_queue_.clear();

    auto close_cb_and_res = closeCompleted();

    VoidResultHandlerFunc window_size_cb;
    window_size_cb.swap(window_size_cb_);

    if (no_more_callbacks_) {
      return;
    }

    // now we are detached from *this* and may be killed from inside callbacks
    // we will call
    auto wptr = weak_from_this();

    if (read_cb_and_res.first) {
      read_cb_and_res.first(read_cb_and_res.second);
    }

    if (wptr.expired() || no_more_callbacks_) {
      return;
    }

    for (const auto &cb : write_callbacks) {
      cb(ec);
      if (wptr.expired() || no_more_callbacks_) {
        return;
      }
    }

    if (window_size_cb) {
      window_size_cb(ec);
    }

    if (wptr.expired() || no_more_callbacks_) {
      return;
    }

    if (close_cb_and_res.first) {
      close_cb_and_res.first(close_cb_and_res.second);
    }
  }

  void YamuxStream::doRead(gsl::span<uint8_t> out, size_t bytes,
                           ReadCallbackFunc cb, bool some) {
    assert(cb);

    if (!cb || bytes == 0 || out.empty()
        || static_cast<size_t>(out.size()) < bytes) {
      return deferReadCallback(Error::STREAM_INVALID_ARGUMENT, std::move(cb));
    }

    // If something is still in read buffer, the client can consume these bytes
    auto bytes_available_now = internal_read_buffer_.size();
    if (bytes_available_now >= bytes || (some && bytes_available_now > 0)) {
      out = out.first(static_cast<ptrdiff_t>(bytes));
      size_t consumed = internal_read_buffer_.consume(out);

      assert(consumed > 0);

      if (is_readable_) {
        feedback_.ackReceivedBytes(stream_id_, consumed);
      }
      return deferReadCallback(consumed, std::move(cb));
    }

    if (close_reason_) {
      return deferReadCallback(close_reason_, std::move(cb));
    }

    if (is_reading_) {
      return deferReadCallback(Error::STREAM_IS_READING, std::move(cb));
    }

    if (!is_readable_) {
      // half closed
      return deferReadCallback(Error::STREAM_NOT_READABLE, std::move(cb));
    }

    is_reading_ = true;
    read_cb_ = std::move(cb);
    external_read_buffer_ = out;
    read_message_size_ = bytes;
    reading_some_ = some;
    external_read_buffer_ =
        external_read_buffer_.first(static_cast<ptrdiff_t>(read_message_size_));

    if (bytes_available_now > 0) {
      internal_read_buffer_.consume(external_read_buffer_);
      external_read_buffer_ = external_read_buffer_.subspan(
          static_cast<ptrdiff_t>(bytes_available_now));
    }
  }

  std::pair<basic::Reader::ReadCallbackFunc, outcome::result<size_t>>
  YamuxStream::readCompleted() {
    using CB = basic::Reader::ReadCallbackFunc;
    std::pair<CB, outcome::result<size_t>> r{CB{}, read_message_size_};
    if (is_reading_) {
      is_reading_ = false;
      read_message_size_ = 0;
      reading_some_ = false;
      if (read_cb_) {
        r.first.swap(read_cb_);
        if (!is_readable_) {
          if (close_reason_) {
            r.second = close_reason_;
          } else {
            // FIN received, but not yet closed
            r.second = Error::STREAM_CLOSED_BY_PEER;
          }
        }
      }
    }
    return r;
  }

  void YamuxStream::doWrite() {
    size_t initial_window_size = window_size_;

    gsl::span<const uint8_t> data;
    bool some = false;
    while (!close_reason_) {
      window_size_ = write_queue_.dequeue(window_size_, data, some);
      if (data.empty()) {
        break;
      }
      TRACE("stream {} dequeued {}/{} bytes to write", stream_id_, data.size(),
            write_queue_.unsentBytes() + data.size());
      feedback_.writeStreamData(stream_id_, data, some);
    }

    if (initial_window_size != window_size_) {
      TRACE("stream {} send window size reduced from {} to {}", stream_id_,
            initial_window_size, window_size_);
    }

    if (!is_writable_ && !close_reason_ && window_size_ > 0) {
      // closing stream for writes, sends FIN
      if (!fin_sent_) {
        fin_sent_ = true;
        feedback_.streamClosed(stream_id_);
      }

      if (!is_readable_) {
        doClose(Error::STREAM_CLOSED_BY_HOST, false);
      } else {
        // let bytes be consumed with peers FIN even if no reader (???)
        peers_window_size_ = maximum_window_size_;
      }
    }
  }

  void YamuxStream::doWrite(gsl::span<const uint8_t> in, size_t bytes,
                            WriteCallbackFunc cb, bool some) {
    if (bytes == 0 || in.empty() || static_cast<size_t>(in.size()) < bytes) {
      return deferWriteCallback(Error::STREAM_INVALID_ARGUMENT, std::move(cb));
    }

    if (!is_writable_) {
      return deferWriteCallback(Error::STREAM_NOT_WRITABLE, std::move(cb));
    }

    if (close_reason_) {
      return deferWriteCallback(close_reason_, std::move(cb));
    }

    if (!write_queue_.canEnqueue(bytes)) {
      return deferWriteCallback(Error::STREAM_WRITE_OVERFLOW, std::move(cb));
    }

    write_queue_.enqueue(in.first(static_cast<ptrdiff_t>(bytes)), some,
                         std::move(cb));
    doWrite();
  }

}  // namespace libp2p::connection
