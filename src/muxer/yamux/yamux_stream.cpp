/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux_stream.hpp>

#include <cassert>

#define TRACE_ENABLED 0
#include <libp2p/common/trace.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, YamuxStream::Error, e) {
  using E = libp2p::connection::YamuxStream::Error;
  switch (e) {
    case E::NOT_READABLE:
      return "the stream is closed for reads";
    case E::NOT_WRITABLE:
      return "the stream is closed for writes";
    case E::INVALID_ARGUMENT:
      return "provided argument is invalid";
    case E::RECEIVE_OVERFLOW:
      return "received unconsumed data amount is greater than it can be";
    case E::IS_WRITING:
      return "there is already a pending write operation on this stream";
    case E::IS_READING:
      return "there is already a pending read operation on this stream";
    case E::INVALID_WINDOW_SIZE:
      return "either window size greater than the maximum one or less than "
             "current number of unconsumed bytes was tried to be set";
    case E::CONNECTION_IS_DEAD:
      return "connection, over which this stream is created, is destroyed";
    case E::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown error";
}

namespace libp2p::connection {

  YamuxStream::YamuxStream(std::weak_ptr<YamuxedConnection> yamuxed_connection,
                           YamuxedConnection::StreamId stream_id,
                           uint32_t maximum_window_size)
      : yamuxed_connection_{std::move(yamuxed_connection)},
        stream_id_{stream_id},
        maximum_window_size_{maximum_window_size} {}

  void YamuxStream::read(gsl::span<uint8_t> out, size_t bytes,
                         ReadCallbackFunc cb) {
    return read(out, bytes, std::move(cb), false);
  }

  void YamuxStream::readSome(gsl::span<uint8_t> out, size_t bytes,
                             ReadCallbackFunc cb) {
    return read(out, bytes, std::move(cb), true);
  }

  void YamuxStream::beginRead(ReadCallbackFunc cb, gsl::span<uint8_t> out,
                              size_t bytes, bool some) {
    assert(!is_reading_);
    assert(!read_cb_);
    TRACE("yamux stream {} beginRead", stream_id_);
    read_cb_ = std::move(cb);
    external_read_buffer_ = out;
    bytes_waiting_ = bytes;
    reading_some_ = some;
    is_reading_ = true;
  }

  void YamuxStream::endRead(outcome::result<size_t> result) {
    TRACE("yamux stream {} endRead", stream_id_);

    // N.B. reentrancy of read_cb_{read} is allowed
    is_reading_ = false;
    bytes_waiting_ = 0;
    reading_some_ = false;
    if (read_cb_) {
      auto cb = std::move(read_cb_);
      read_cb_ = ReadCallbackFunc{};
      cb(result);
    }
  }

  outcome::result<size_t> YamuxStream::tryConsumeReadBuffer(
      gsl::span<uint8_t> out, size_t bytes, bool some) {
    // will try to consume n bytes if applicable
    auto n = std::min(read_buffer_.size(), bytes);

    TRACE("stream {}: need {} bytes, available {} bytes", stream_id_, bytes, n);

    if ((some && n > 0) || (!some && n == bytes)) {
      auto copied = boost::asio::buffer_copy(boost::asio::buffer(out.data(), n),
                                             read_buffer_.data(), n);
      if (copied != n) {
        return Error::INTERNAL_ERROR;
      }

      sendAck(n);
      return n;
    }

    // cannot consume required bytes from existing read buffer
    return 0;
  }

  void YamuxStream::sendAck(size_t bytes) {
    read_buffer_.consume(bytes);
    receive_window_size_ += bytes;

    if (!is_readable_ || yamuxed_connection_.expired()) {
      return;
    }
    yamuxed_connection_.lock()->streamAckBytes(
        stream_id_, bytes,
        [self{shared_from_this()}](outcome::result<void> res) {
          if (!res) {
            return self->onConnectionReset(res.error());
          }
        });
  }

  void YamuxStream::read(gsl::span<uint8_t> out, size_t bytes,
                         ReadCallbackFunc cb, bool some) {
    assert(cb);

    if (!cb || bytes == 0 || out.empty()
        || static_cast<size_t>(out.size()) < bytes) {
      return cb(Error::INVALID_ARGUMENT);
    }

    if (is_reading_) {
      return cb(Error::IS_READING);
    }

    auto res = tryConsumeReadBuffer(out, bytes, some);
    if (!res || res.value() > 0) {
      return cb(res);
    }

    // is_readable_ flag is set due to FIN flag from the other side.
    // Nevertheless, unconsumed data may exist at the moment
    if (!is_readable_) {
      return endRead(Error::NOT_READABLE);
    }

    if (yamuxed_connection_.expired()) {
      return endRead(Error::CONNECTION_IS_DEAD);
    }

    // cannot return immediately, wait for incoming data
    beginRead(std::move(cb), out, bytes, some);
  }

  void YamuxStream::write(gsl::span<const uint8_t> in, size_t bytes,
                          WriteCallbackFunc cb) {
    return write(in, bytes, std::move(cb), false);
  }

  void YamuxStream::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                              WriteCallbackFunc cb) {
    return write(in, bytes, std::move(cb), true);
  }

  void YamuxStream::beginWrite(WriteCallbackFunc cb) {
    assert(!is_writing_);
    assert(!write_cb_);
    TRACE("yamux stream {} beginWrite", stream_id_);
    write_cb_ = std::move(cb);
    is_writing_ = true;
  }

  void YamuxStream::endWrite(outcome::result<size_t> result) {
    TRACE("yamux stream {} endWrite", stream_id_);

    // N.B. reentrancy of write_cb_{write} is allowed
    is_writing_ = false;
    if (write_cb_) {
      auto cb = std::move(write_cb_);
      write_cb_ = WriteCallbackFunc{};
      cb(result);
    }

    std::lock_guard<std::mutex> lock(write_queue_mutex_);
    // check if new write messages were received while stream was writing
    // and propagate these messages
    if (not write_queue_.empty()) {
      auto [in, bytes, cb, some] = write_queue_.front();
      write_queue_.pop_front();
      write(in, bytes, cb, some);
    }
  }

  void YamuxStream::write(gsl::span<const uint8_t> in, size_t bytes,
                          WriteCallbackFunc cb, bool some) {
    if (!is_writable_) {
      return cb(Error::NOT_WRITABLE);
    }
    if (is_writing_) {
      std::lock_guard<std::mutex> lock(write_queue_mutex_);

      std::vector<uint8_t> in_vector(in.begin(), in.end());
      write_queue_.emplace_back(in_vector, bytes, cb, some);
      return;
    }

    beginWrite(std::move(cb));

    auto write_lambda = [self{shared_from_this()}, bytes,
                         some](gsl::span<const uint8_t> in) mutable -> bool {
      if (self->send_window_size_ >= bytes) {
        // we can write - window size on the other side allows us
        auto conn_wptr = self->yamuxed_connection_;
        if (conn_wptr.expired()) {
          self->endWrite(Error::CONNECTION_IS_DEAD);
        } else {
          conn_wptr.lock()->streamWrite(self->stream_id_, in, bytes, some,
                                        [self](outcome::result<size_t> res) {
                                          if (res) {
                                            self->send_window_size_ -=
                                                res.value();
                                          }
                                          self->endWrite(res);
                                        });
        }
        return true;
      }
      return false;
    };

    // if we can write now - do it and return
    if (write_lambda(in)) {
      return;
    }

    // else, subscribe to window updates, so that when the window gets wide
    // enough, we could write
    if (yamuxed_connection_.expired()) {
      return endWrite(Error::CONNECTION_IS_DEAD);
    }
    yamuxed_connection_.lock()->streamOnWindowUpdate(
        stream_id_,
        [write_lambda = std::move(write_lambda),
         in_bytes = std::vector<uint8_t>{in.begin(), in.end()}]() mutable {
          return write_lambda(in_bytes);
        });
  }

  bool YamuxStream::isClosed() const noexcept {
    return !is_readable_ && !is_writable_;
  }

  void YamuxStream::close(VoidResultHandlerFunc cb) {
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    is_writing_ = true;
    if (yamuxed_connection_.expired()) {
      return cb(Error::CONNECTION_IS_DEAD);
    }
    yamuxed_connection_.lock()->streamClose(
        stream_id_, [self{shared_from_this()}, cb = std::move(cb)](auto &&res) {
          self->is_writing_ = false;
          cb(std::forward<decltype(res)>(res));
        });
  }

  bool YamuxStream::isClosedForRead() const noexcept {
    return !is_readable_;
  }

  bool YamuxStream::isClosedForWrite() const noexcept {
    return !is_writable_;
  }

  void YamuxStream::reset() {
    if (is_writing_) {
      return;
    }

    is_writing_ = true;
    if (yamuxed_connection_.expired()) {
      return;
    }
    yamuxed_connection_.lock()->streamReset(
        stream_id_, [self{shared_from_this()}](auto && /*ignore*/) {
          self->is_writing_ = false;
          self->resetStream();
        });
  }

  void YamuxStream::adjustWindowSize(uint32_t new_size,
                                     VoidResultHandlerFunc cb) {
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    if (new_size > maximum_window_size_ || new_size < read_buffer_.size()) {
      return cb(Error::INVALID_WINDOW_SIZE);
    }

    is_writing_ = true;
    if (yamuxed_connection_.expired()) {
      return cb(Error::CONNECTION_IS_DEAD);
    }
    yamuxed_connection_.lock()->streamAckBytes(
        stream_id_, new_size - receive_window_size_,
        [self{shared_from_this()}, cb = std::move(cb), new_size](auto &&res) {
          self->is_writing_ = false;
          if (!res) {
            return cb(res.error());
          }
          self->receive_window_size_ = new_size;
          cb(outcome::success());
        });
  }

  outcome::result<peer::PeerId> YamuxStream::remotePeerId() const {
    if (auto conn = yamuxed_connection_.lock()) {
      return conn->remotePeer();
    }
    return Error::CONNECTION_IS_DEAD;
  }

  outcome::result<bool> YamuxStream::isInitiator() const {
    if (auto conn = yamuxed_connection_.lock()) {
      return conn->isInitiator();
    }
    return Error::CONNECTION_IS_DEAD;
  }

  outcome::result<multi::Multiaddress> YamuxStream::localMultiaddr() const {
    if (auto conn = yamuxed_connection_.lock()) {
      return conn->localMultiaddr();
    }
    return Error::CONNECTION_IS_DEAD;
  }

  outcome::result<multi::Multiaddress> YamuxStream::remoteMultiaddr() const {
    if (auto conn = yamuxed_connection_.lock()) {
      return conn->remoteMultiaddr();
    }
    return Error::CONNECTION_IS_DEAD;
  }

  void YamuxStream::resetStream() {
    is_readable_ = false;
    is_writable_ = false;
  }

  outcome::result<void> YamuxStream::commitData(gsl::span<const uint8_t> data,
                                                size_t data_size) {
    if (data_size > receive_window_size_) {
      return Error::RECEIVE_OVERFLOW;
    }

    size_t bytes_remain = data_size;
    bool inplace_readop = false;

    if (read_buffer_.size() == 0 && bytes_waiting_ > 0) {
      // will try to consume n bytes w/o copying to intermediate buffer
      auto n = std::min(data_size, bytes_waiting_);

      TRACE("stream {}: need {} bytes, available {} bytes", stream_id_,
            bytes_waiting_, n);

      if ((reading_some_ && n > 0) || (!reading_some_ && n == bytes_waiting_)) {
        memcpy(external_read_buffer_.data(), data.data(), n);
        sendAck(n);
        bytes_remain -= n;
        inplace_readop = true;
      }
    }

    if (bytes_remain > 0) {
      if (boost::asio::buffer_copy(
              read_buffer_.prepare(bytes_remain),
              boost::asio::const_buffer(data.data() + data_size - bytes_remain,
                                        bytes_remain))
          != bytes_remain) {
        return Error::INTERNAL_ERROR;
      }
      read_buffer_.commit(bytes_remain);
    }

    receive_window_size_ -= data_size;

    if (inplace_readop) {
      assert(read_cb_);
      assert(data_size - bytes_remain > 0);
      endRead(data_size - bytes_remain);
    } else if (bytes_waiting_ > 0) {
      assert(read_cb_);
      auto res = tryConsumeReadBuffer(external_read_buffer_, bytes_waiting_,
                                      reading_some_);
      if (!res || res.value() > 0) {
        endRead(res);
      }
    }

    return outcome::success();
  }

  void YamuxStream::onConnectionReset(outcome::result<size_t> reason) {
    assert(reason.has_error());

    resetStream();
    endRead(reason);
    endWrite(reason);
  }

}  // namespace libp2p::connection
