/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux_stream.hpp>

#include <cassert>

#include <libp2p/common/logger.hpp>
#include <libp2p/muxer/yamux/yamux_error.hpp>

namespace libp2p::connection {

  namespace {
    auto log() {
      static auto logger = common::createLogger("yx-stream");
      return logger.get();
    }
  }  // namespace

  YamuxStream::YamuxStream(
      std::shared_ptr<connection::SecureConnection> connection,
      YamuxStreamFeedback &feedback, uint32_t stream_id, size_t window_size,
      size_t maximum_window_size, size_t write_queue_limit)
      : connection_(std::move(connection)),
        feedback_(feedback),
        stream_id_(stream_id),
        send_window_size_(window_size),
        receive_window_size_(window_size),
        maximum_window_size_(maximum_window_size),
        write_queue_(write_queue_limit) {
    assert(connection_);
    assert(stream_id_ > 0);
    assert(send_window_size_ <= maximum_window_size_);
    assert(receive_window_size_ <= maximum_window_size_);
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
    feedback_.deferCall([wptr = weak_from_this(), res, cb = std::move(cb)]() {
      auto self = wptr.lock();
      if (self && !self->closed_by_client_) {
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
    feedback_.deferCall([wptr = weak_from_this(), ec, cb = std::move(cb)]() {
      auto self = wptr.lock();
      if (self && !self->closed_by_client_) {
        cb(ec);
      }
    });
  }

  bool YamuxStream::isClosed() const noexcept {
    return close_reason_.value() != 0;
  }

  void YamuxStream::close(VoidResultHandlerFunc cb) {
    closed_by_client_ = true;
    is_writable_ = false;

    feedback_.resetStream(stream_id_, false);

    doClose(YamuxError::STREAM_CLOSED_BY_HOST, false);

    // TODO send FIN instead make close_cb_

    if (cb) {
      feedback_.deferCall([wptr = weak_from_this(), cb = std::move(cb)]() {
        if (!wptr.expired()) {
          cb(outcome::success());
        }
      });
    }
  }

  bool YamuxStream::isClosedForRead() const noexcept {
    return !is_readable_;
  }

  bool YamuxStream::isClosedForWrite() const noexcept {
    return !is_writable_;
  }

  void YamuxStream::reset() {
    closed_by_client_ = true;

    feedback_.resetStream(stream_id_, true);

    doClose(YamuxError::STREAM_RESET_BY_HOST, false);
  }

  void YamuxStream::adjustWindowSize(uint32_t new_size,
                                     VoidResultHandlerFunc cb) {
    if (new_size > maximum_window_size_ || new_size < receive_window_size_) {
      if (cb) {
        feedback_.deferCall([wptr = weak_from_this(), cb = std::move(cb)]() {
          auto self = wptr.lock();
          if (!self) {
            return;
          }
          if (!self->close_reason_) {
            cb(YamuxError::INVALID_WINDOW_SIZE);
          } else {
            cb(self->close_reason_);
          }
        });
      }
      return;
    }

    feedback_.ackReceivedBytes(stream_id_, new_size - receive_window_size_);

    if (cb) {
      window_size_cb_ = [wptr = weak_from_this(), cb = std::move(cb),
                         new_size](auto &&res) {
        auto self = wptr.lock();
        if (!self) {
          return;
        }
        if (!self->close_reason_) {
          cb(self->close_reason_);
        } else if (self->receive_window_size_ >= new_size) {
          cb(outcome::success());
        } else {
          // continue waiting
          return;
        }
        self->window_size_cb_ = VoidResultHandlerFunc{};
      };
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
    send_window_size_ += delta;
    doWrite();
  }

  bool YamuxStream::onDataRead(gsl::span<uint8_t> bytes, bool fin, bool rst) {
    if (close_reason_) {
      return true;
    }

    auto sz = static_cast<size_t>(bytes.size());

    bool read_completed = false;
    bool overflow = false;

    if (sz > 0) {
      overflow = receive_window_size_
          < (internal_read_buffer_.size() + external_read_buffer_.size()
             + bytes.size());

      if (is_reading_) {
        auto bytes_needed = static_cast<size_t>(external_read_buffer_.size());
        size_t consumed =
            internal_read_buffer_.addAndConsume(bytes, external_read_buffer_);
        if (reading_some_) {
          read_completed = true;
          read_message_size_ = consumed;
        } else if (bytes_needed == consumed) {
          read_completed = true;
        } else {
          assert(consumed < bytes_needed);

          external_read_buffer_ = external_read_buffer_.subspan(consumed);
        }
      }
    }

    if (fin) {
      is_readable_ = false;
    }

    if (overflow) {
      doClose(YamuxError::RECEIVE_WINDOW_OVERFLOW, false);
    } else if (rst) {
      doClose(YamuxError::STREAM_RESET_BY_PEER, false);
    } else if (sz > 0) {
      feedback_.ackReceivedBytes(stream_id_, sz);
      receive_window_size_ += sz;
      if (window_size_cb_) {
        window_size_cb_(outcome::success());
      }
    }

    if (read_completed) {
      readCompleted();
    }

    return !overflow;
  }

  void YamuxStream::onDataWritten(size_t bytes) {
    if (!write_queue_.ack(bytes)) {
      log()->error("write queue ack failed");
      feedback_.resetStream(stream_id_, true);
      doClose(YamuxError::INTERNAL_ERROR, true);
    }
  }

  void YamuxStream::closedByConnection(std::error_code ec) {
    doClose(ec, true);
  }

  void YamuxStream::doClose(std::error_code ec, bool notify_read_callback) {
    assert(ec);

    close_reason_ = ec;
    is_readable_ = false;

    if (notify_read_callback) {
      internal_read_buffer_.clear();
      if (is_reading_) {
        assert(!closed_by_client_);

        is_reading_ = false;
        assert(read_cb_);
        auto cb = std::move(read_cb_);
        read_cb_ = ReadCallbackFunc{};
        cb(ec);
      }
    }

    if (!closed_by_client_) {
      write_queue_.broadcast(
          [this](const basic::Writer::WriteCallbackFunc &cb) -> bool {
            if (!closed_by_client_) {
              cb(close_reason_);
            }

            // reentrancy may occur here
            return !closed_by_client_;
          });

      write_queue_.clear();
    }
  }

  void YamuxStream::doRead(gsl::span<uint8_t> out, size_t bytes,
                           ReadCallbackFunc cb, bool some) {
    assert(cb);

    if (!cb || bytes == 0 || out.empty()
        || static_cast<size_t>(out.size()) < bytes) {
      return deferReadCallback(YamuxError::INVALID_ARGUMENT, std::move(cb));
    }

    if (close_reason_) {
      return deferReadCallback(close_reason_, std::move(cb));
    }

    if (is_reading_) {
      return deferReadCallback(YamuxError::STREAM_IS_READING, std::move(cb));
    }

    if (!is_readable_) {
      // half closed
      return deferReadCallback(YamuxError::STREAM_NOT_READABLE, std::move(cb));
    }

    is_reading_ = true;
    read_cb_ = std::move(cb);
    external_read_buffer_ = out;
    read_message_size_ = bytes;
    reading_some_ = some;
    external_read_buffer_ = external_read_buffer_.first(read_message_size_);

    size_t consumed = internal_read_buffer_.consume(external_read_buffer_);
    if (consumed > 0) {
      if (some) {
        read_message_size_ = consumed;
      }
      external_read_buffer_ = external_read_buffer_.subspan(consumed);
    }

    if (external_read_buffer_.empty()) {
      // read completed, defer callback
      deferReadCallback(read_message_size_, [this](auto) { readCompleted(); });
    }
  }

  void YamuxStream::readCompleted() {
    if (is_reading_) {
      is_reading_ = false;
      size_t read_message_size = read_message_size_;
      read_message_size_ = 0;
      reading_some_ = false;
      if (read_cb_) {
        auto cb = std::move(read_cb_);
        read_cb_ = ReadCallbackFunc{};
        cb(read_message_size);
      }
    }
  }

  void YamuxStream::doWrite() {
    gsl::span<const uint8_t> data;
    bool some = false;
    while (send_window_size_ > 0 && !close_reason_) {
      send_window_size_ = write_queue_.dequeue(send_window_size_, data, some);
      if (data.empty()) {
        break;
      }
      feedback_.writeStreamData(stream_id_, data, some);
    }
  }

  void YamuxStream::doWrite(gsl::span<const uint8_t> in, size_t bytes,
                            WriteCallbackFunc cb, bool some) {
    if (bytes == 0 || in.empty() || static_cast<size_t>(in.size()) < bytes) {
      return deferWriteCallback(YamuxError::INVALID_ARGUMENT, std::move(cb));
    }

    if (!is_writable_) {
      return deferWriteCallback(YamuxError::STREAM_NOT_WRITABLE, std::move(cb));
    }

    if (close_reason_) {
      return deferWriteCallback(close_reason_, std::move(cb));
    }

    if (!write_queue_.canEnqueue(bytes)) {
      return deferWriteCallback(YamuxError::STREAM_WRITE_BUFFER_OVERFLOW,
                                std::move(cb));
    }

    write_queue_.enqueue(in.first(bytes), some, std::move(cb));
    doWrite();
  }

}  // namespace libp2p::connection
