/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux_stream.hpp>

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

  void YamuxStream::read(gsl::span<uint8_t> out, size_t bytes,
                         ReadCallbackFunc cb, bool some) {
    if (bytes == 0 || out.empty() || static_cast<size_t>(out.size()) < bytes) {
      return cb(Error::INVALID_ARGUMENT);
    }
    if (!is_readable_) {
      return cb(Error::NOT_READABLE);
    }
    if (is_reading_) {
      return cb(Error::IS_READING);
    }

    is_reading_ = true;

    auto read_lambda = [self{shared_from_this()}, cb = std::move(cb), out,
                        bytes, some]() mutable {
      // if there is enough data in our buffer (depending if we want to read
      // some or all bytes), read it
      if ((some && self->read_buffer_.size() != 0)
          || (!some && self->read_buffer_.size() >= bytes)) {
        auto to_read =
            some ? std::min(self->read_buffer_.size(), bytes) : bytes;
        if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), to_read),
                                     self->read_buffer_.data(), to_read)
            != to_read) {
          cb(Error::INTERNAL_ERROR);
        } else {
          auto conn_wptr = self->yamuxed_connection_;
          if (conn_wptr.expired()) {
            cb(Error::CONNECTION_IS_DEAD);
          } else {
            conn_wptr.lock()->streamAckBytes(
                self->stream_id_, to_read,
                [self, cb = std::move(cb), to_read](auto &&res) {
                  self->is_reading_ = false;
                  if (!res) {
                    return cb(res.error());
                  }
                  self->read_buffer_.consume(to_read);
                  self->receive_window_size_ += to_read;
                  cb(to_read);
                });
          }
        }
        return true;
      }
      return false;
    };

    // return immediately, if there's enough data in the buffer
    if (read_lambda()) {
      return;
    }

    // else, set a callback, which is called each time a new data arrives
    if (yamuxed_connection_.expired()) {
      return cb(Error::CONNECTION_IS_DEAD);
    }
    yamuxed_connection_.lock()->streamOnAddData(
        stream_id_, [read_lambda = std::move(read_lambda)]() mutable {
          return read_lambda();
        });
  }

  void YamuxStream::write(gsl::span<const uint8_t> in, size_t bytes,
                          WriteCallbackFunc cb) {
    return write(in, bytes, std::move(cb), false);
  }

  void YamuxStream::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                              WriteCallbackFunc cb) {
    return write(in, bytes, std::move(cb), true);
  }

  void YamuxStream::write(gsl::span<const uint8_t> in, size_t bytes,
                          WriteCallbackFunc cb, bool some) {
    if (!is_writable_) {
      return cb(Error::NOT_WRITABLE);
    }
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    is_writing_ = true;

    auto write_lambda = [self{shared_from_this()}, cb = std::move(cb), in,
                         bytes, some]() mutable {
      if (self->send_window_size_ >= bytes) {
        // we can write - window size on the other side allows us
        auto conn_wptr = self->yamuxed_connection_;
        if (conn_wptr.expired()) {
          cb(Error::CONNECTION_IS_DEAD);
        } else {
          conn_wptr.lock()->streamWrite(
              self->stream_id_, in, bytes, some,
              [self, cb = std::move(cb)](auto &&res) {
                self->is_writing_ = false;
                if (res) {
                  self->send_window_size_ -= res.value();
                }
                return cb(std::forward<decltype(res)>(res));
              });
        }
        return true;
      }
      return false;
    };

    // if we can write now - do it and return
    if (write_lambda()) {
      return;
    }

    // else, subscribe to window updates, so that when the window gets wide
    // enough, we could write
    if (yamuxed_connection_.expired()) {
      return cb(Error::CONNECTION_IS_DEAD);
    }
    yamuxed_connection_.lock()->streamOnWindowUpdate(
        stream_id_, [write_lambda = std::move(write_lambda)]() mutable {
          return write_lambda();
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

    if (boost::asio::buffer_copy(
            read_buffer_.prepare(data_size),
            boost::asio::const_buffer(data.data(), data_size))
        != data_size) {
      return Error::INTERNAL_ERROR;
    }
    read_buffer_.commit(data_size);

    receive_window_size_ -= data_size;

    return outcome::success();
  }

}  // namespace libp2p::connection
