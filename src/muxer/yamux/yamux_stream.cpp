/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux_stream.hpp>

#include <cassert>

#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/common/ambigous_size.hpp>
#include <libp2p/muxer/yamux/yamux_frame.hpp>
#include <qtils/option_take.hpp>

#define TRACE_ENABLED 0
#include <libp2p/common/trace.hpp>

namespace libp2p::connection {

  namespace {
    auto log() {
      static auto logger = log::createLogger("yx-stream");
      return logger.get();
    }
  }  // namespace

  struct FinallyReading {
    FinallyReading(YamuxStream::ReadCallbackFunc cb, outcome::result<size_t> r)
        : cb{std::move(cb)}, r{r} {}
    ~FinallyReading() {
      cb(r);
    }

    // clang-tidy cppcoreguidelines-special-member-functions
    FinallyReading(const FinallyReading &) = delete;
    void operator=(const FinallyReading &) = delete;
    FinallyReading(FinallyReading &&) = delete;
    void operator=(FinallyReading &&) = delete;

    YamuxStream::ReadCallbackFunc cb;
    outcome::result<size_t> r;
  };

  YamuxStream::YamuxStream(
      std::shared_ptr<connection::SecureConnection> connection,
      YamuxStreamFeedback &feedback,
      uint32_t stream_id,
      size_t maximum_window_size,
      size_t write_queue_limit)
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

  void YamuxStream::read(BytesOut out, size_t bytes, ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    readReturnSize(shared_from_this(), out, std::move(cb));
  }

  void YamuxStream::readSome(BytesOut out, size_t bytes, ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    doRead(out, std::move(cb));
  }

  void YamuxStream::deferReadCallback(outcome::result<size_t> res,
                                      ReadCallbackFunc cb) {
    feedback_.deferCall([res, cb{std::move(cb)}] { cb(res); });
  }

  void YamuxStream::writeSome(BytesIn in, size_t bytes, WriteCallbackFunc cb) {
    ambigousSize(in, bytes);
    doWrite(in, std::move(cb));
  }

  void YamuxStream::deferWriteCallback(std::error_code ec,
                                       WriteCallbackFunc cb) {
    feedback_.deferCall([ec, cb{std::move(cb)}] { cb(ec); });
  }

  bool YamuxStream::isClosed() const {
    return close_reason_.has_value();
  }

  void YamuxStream::close(VoidResultHandlerFunc cb) {
    if (isClosed()) {
      if (cb) {
        feedback_.deferCall(
            [cb{std::move(cb)}, ec{*close_reason_}] { cb(ec); });
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
      p.second = *close_reason_;
    }
    if (close_cb_) {
      p.first.swap(close_cb_);
    }
    return p;
  }

  bool YamuxStream::isClosedForRead() const {
    return !is_readable_;
  }

  bool YamuxStream::isClosedForWrite() const {
    return !is_writable_;
  }

  void YamuxStream::reset() {
    feedback_.resetStream(stream_id_);
    doClose(Error::STREAM_RESET_BY_HOST);
  }

  void YamuxStream::adjustWindowSize(uint32_t new_size,
                                     VoidResultHandlerFunc cb) {
    auto ec = close_reason_;
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
      feedback_.deferCall([cb{std::move(cb)}, ec] {
        if (!ec) {
          cb(outcome::success());
        } else {
          cb(*ec);
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
      TRACE("stream {} send window increased by {} to {}",
            stream_id_,
            delta,
            window_size_);
      doWrite();
    }
  }

  YamuxStream::DataFromConnectionResult YamuxStream::onDataReceived(
      BytesOut bytes) {
    auto sz = static_cast<size_t>(bytes.size());

    if (sz == 0) {
      log()->critical("zero data packet received - should not get here");
      return kKeepStream;
    }

    TRACE("stream {} read {} bytes", stream_id_, sz);

    bool overflow = false;
    size_t bytes_consumed = 0;
    std::optional<FinallyReading> finally_reading;

    // First transfer bytes to client if available
    if (auto reading = qtils::optionTake(reading_)) {
      assert(internal_read_buffer_.empty());

      // if sz > bytes_needed then internal buffer will be non empty after
      // this
      bytes_consumed = internal_read_buffer_.addAndConsume(bytes, reading->out);
      assert(bytes_consumed > 0);
      finally_reading.emplace(std::move(reading->cb), bytes_consumed);
    } else {
      internal_read_buffer_.add(bytes);
    }

    if (!internal_read_buffer_.empty()) {
      overflow = (internal_read_buffer_.size() > peers_window_size_);
      if (overflow) {
        log()->debug("read buffer overflow {} > {}, stream {}",
                     internal_read_buffer_.size(),
                     peers_window_size_,
                     stream_id_);
      } else {
        TRACE("stream {} receive window reduced by {} to {}",
              stream_id_,
              internal_read_buffer_.size(),
              peers_window_size_ - internal_read_buffer_.size());
      }
    }

    if (isClosed()) {
      // already closed, maybe error
      return kRemoveStreamAndSendRst;
    }

    if (overflow) {
      doClose(Error::STREAM_RECEIVE_OVERFLOW);
    } else if (bytes_consumed > 0) {
      feedback_.ackReceivedBytes(stream_id_, bytes_consumed);
      TRACE("stream {} receive window increased by {} to {}",
            stream_id_,
            bytes_consumed,
            peers_window_size_ - internal_read_buffer_.size());
    }

    return overflow ? kRemoveStreamAndSendRst : kKeepStream;
  }

  YamuxStream::DataFromConnectionResult YamuxStream::onFINReceived() {
    std::optional<FinallyReading> finally_reading;
    if (auto reading = qtils::optionTake(reading_)) {
      finally_reading.emplace(std::move(reading->cb),
                              Error::STREAM_CLOSED_BY_HOST);
    }

    if (isClosed()) {
      // already closed, maybe error
      return kRemoveStreamAndSendRst;
    }

    is_readable_ = false;

    if (!is_writable_) {
      doClose(Error::STREAM_CLOSED_BY_HOST);

      // connection will remove stream
      return kRemoveStream;
    }

    return kKeepStream;
  }

  void YamuxStream::onRSTReceived() {
    if (isClosed()) {
      // already closed, maybe error
      return;
    }

    doClose(Error::STREAM_RESET_BY_PEER);
  }

  void YamuxStream::onDataWritten(size_t bytes) {
    auto result = write_queue_.ackDataSent(bytes);
    if (!result.data_consistent) {
      log()->error("write queue ack failed, stream {}", stream_id_);
      feedback_.resetStream(stream_id_);
      doClose(Error::STREAM_INTERNAL_ERROR);
      return;
    }

    if (result.cb) {
      result.cb(result.size_to_ack);
    }
  }

  void YamuxStream::closedByConnection(std::error_code ec) {
    doClose(std::move(ec));
  }

  void YamuxStream::doClose(std::error_code ec) {
    // ensure lifetime of this object during doClose
    auto self = shared_from_this();

    std::optional<FinallyReading> finally_reading;
    if (auto reading = qtils::optionTake(reading_)) {
      finally_reading.emplace(std::move(reading->cb), ec);
    }

    if (close_reason_) {
      // already closed
      return;
    }

    close_reason_ = ec;
    is_readable_ = false;
    is_writable_ = false;

    internal_read_buffer_.clear();

    auto write_callbacks = write_queue_.getAllCallbacks();

    write_queue_.clear();

    auto close_cb_and_res = closeCompleted();

    VoidResultHandlerFunc window_size_cb;
    window_size_cb.swap(window_size_cb_);

    for (const auto &cb : write_callbacks) {
      cb(ec);
    }

    if (window_size_cb) {
      window_size_cb(ec);
    }

    if (close_cb_and_res.first) {
      close_cb_and_res.first(close_cb_and_res.second);
    }
  }

  void YamuxStream::doRead(BytesOut out, ReadCallbackFunc cb) {
    assert(cb);

    if (out.empty()) {
      return deferReadCallback(Error::STREAM_INVALID_ARGUMENT, std::move(cb));
    }

    // If something is still in read buffer, the client can consume these bytes
    auto bytes_available_now = internal_read_buffer_.size();
    if (bytes_available_now > 0) {
      size_t consumed = internal_read_buffer_.consume(out);

      assert(consumed > 0);

      if (is_readable_) {
        feedback_.ackReceivedBytes(stream_id_, consumed);
      }
      return deferReadCallback(consumed, std::move(cb));
    }

    if (close_reason_) {
      return deferReadCallback(*close_reason_, std::move(cb));
    }

    if (reading_.has_value()) {
      abort();
    }

    if (!is_readable_) {
      // half closed
      return deferReadCallback(Error::STREAM_NOT_READABLE, std::move(cb));
    }

    reading_.emplace(out, std::move(cb));
  }

  void YamuxStream::doWrite() {
    size_t initial_window_size = window_size_;

    BytesIn data;
    while (!close_reason_) {
      window_size_ = write_queue_.dequeue(window_size_, data);
      if (data.empty()) {
        break;
      }
      TRACE("stream {} dequeued {}/{} bytes to write",
            stream_id_,
            data.size(),
            write_queue_.unsentBytes() + data.size());
      feedback_.writeStreamData(stream_id_, data);
    }

    if (initial_window_size != window_size_) {
      TRACE("stream {} send window size reduced from {} to {}",
            stream_id_,
            initial_window_size,
            window_size_);
    }

    if (!is_writable_ && !close_reason_ && window_size_ > 0) {
      // closing stream for writes, sends FIN
      if (!fin_sent_) {
        fin_sent_ = true;
        feedback_.streamClosed(stream_id_);
      }

      if (!is_readable_) {
        doClose(Error::STREAM_CLOSED_BY_HOST);
      } else {
        // let bytes be consumed with peers FIN even if no reader (???)
        peers_window_size_ = maximum_window_size_;
      }
    }
  }

  void YamuxStream::doWrite(BytesIn in, WriteCallbackFunc cb) {
    if (in.empty()) {
      return deferWriteCallback(Error::STREAM_INVALID_ARGUMENT, std::move(cb));
    }

    if (!is_writable_) {
      return deferWriteCallback(Error::STREAM_NOT_WRITABLE, std::move(cb));
    }

    if (close_reason_) {
      return deferWriteCallback(
          std::error_code{},
          [cb{std::move(cb)}, res{*close_reason_}](
              outcome::result<size_t>) mutable { cb(std::move(res)); });
    }

    if (!write_queue_.canEnqueue(in.size())) {
      return deferWriteCallback(Error::STREAM_WRITE_OVERFLOW, std::move(cb));
    }

    write_queue_.enqueue(in, std::move(cb));
    doWrite();
  }

}  // namespace libp2p::connection
