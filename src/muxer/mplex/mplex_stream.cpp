/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/mplex/mplex_stream.hpp>

#include <algorithm>

#include <boost/container_hash/hash.hpp>
#include <libp2p/muxer/mplex/mplexed_connection.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, MplexStream::Error, e) {
  using E = libp2p::connection::MplexStream::Error;
  switch (e) {
    case E::CONNECTION_IS_DEAD:
      return "underlying connection is closed or deleted";
    case E::BAD_WINDOW_SIZE:
      return "bad windows size was passed for adjusting";
    case E::INVALID_ARGUMENT:
      return "invalid argument was passed";
    case E::IS_READING:
      return "read was called before the last read completed";
    case E::IS_CLOSED_FOR_READS:
      return "this stream is closed for reads";
    case E::IS_WRITING:
      return "write was called before the last write completed";
    case E::IS_CLOSED_FOR_WRITES:
      return "this stream is closed for writes";
    case E::IS_RESET:
      return "this stream was reset";
    case E::RECEIVE_OVERFLOW:
      return "stream received more data than it is allowed by the window size";
    case E::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown error";
}

#define TRY_GET_CONNECTION(conn_var_name) \
  if (connection_.expired()) {            \
    return Error::CONNECTION_IS_DEAD;     \
  }                                       \
  auto(conn_var_name) = connection_.lock();

namespace libp2p::connection {
  std::string MplexStream::StreamId::toString() const {
    auto initiator_str = initiator ? "true" : "false";
    return "StreamId{" + std::to_string(number) + ", " + initiator_str + "}";
  }

  bool MplexStream::StreamId::operator==(const StreamId &other) const {
    return number == other.number && initiator == other.initiator;
  }

  MplexStream::MplexStream(std::weak_ptr<MplexedConnection> connection,
                           StreamId stream_id)
      : connection_{std::move(connection)}, stream_id_{stream_id} {}

  void MplexStream::read(gsl::span<uint8_t> out, size_t bytes,
                         ReadCallbackFunc cb) {
    read(out, bytes, std::move(cb), false);
  }

  void MplexStream::readSome(gsl::span<uint8_t> out, size_t bytes,
                             ReadCallbackFunc cb) {
    read(out, bytes, std::move(cb), true);
  }

  void MplexStream::read(gsl::span<uint8_t> out, size_t bytes,
                         ReadCallbackFunc cb, bool some) {
    if (is_reset_) {
      return cb(Error::IS_RESET);
    }
    if (!is_readable_) {
      return cb(Error::IS_CLOSED_FOR_READS);
    }
    if (bytes == 0 || out.empty() || static_cast<size_t>(out.size()) < bytes) {
      return cb(Error::INVALID_ARGUMENT);
    }
    if (is_reading_) {
      return cb(Error::IS_READING);
    }

    // this lambda checks, if there's enough data in our read buffer, and gives
    // it to the caller, if so
    auto read_lambda = [self{shared_from_this()}, cb{std::move(cb)}, out, bytes,
                        some](outcome::result<size_t> res) mutable {
      if (!res) {
        self->data_notified_ = true;
        return cb(res);
      }
      if ((some && self->read_buffer_.size() != 0)
          || (!some && self->read_buffer_.size() >= bytes)) {
        auto to_read =
            some ? std::min(self->read_buffer_.size(), bytes) : bytes;
        if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), to_read),
                                     self->read_buffer_.data(), to_read)
            != to_read) {
          return cb(Error::INTERNAL_ERROR);
        }

        self->is_reading_ = false;
        self->read_buffer_.consume(to_read);
        self->receive_window_size_ += to_read;
        self->data_notified_ = true;
        cb(to_read);
      }
    };

    // return immediately, if there's enough data in the buffer
    data_notified_ = false;
    read_lambda(0);
    if (data_notified_) {
      return;
    }

    if (connection_.expired()) {
      return cb(Error::CONNECTION_IS_DEAD);
    }

    // subscribe to new data updates
    is_reading_ = true;
    data_notifyee_ = std::move(read_lambda);
  }

  void MplexStream::write(gsl::span<const uint8_t> in, size_t bytes,
                          WriteCallbackFunc cb) {
    if (is_reset_) {
      return cb(Error::IS_RESET);
    }
    if (!is_writable_) {
      return cb(Error::IS_CLOSED_FOR_WRITES);
    }
    if (bytes == 0 || in.empty() || static_cast<size_t>(in.size()) < bytes) {
      return cb(Error::INVALID_ARGUMENT);
    }
    if (is_writing_) {
      std::vector<uint8_t> in_vector(in.begin(), in.end());
      std::lock_guard<std::mutex> lock(write_queue_mutex_);
      write_queue_.emplace_back(in_vector, bytes, cb);
      return;
    }
    if (connection_.expired()) {
      return cb(Error::CONNECTION_IS_DEAD);
    }

    is_writing_ = true;
    connection_.lock()->streamWrite(
        stream_id_, in, bytes,
        [self{shared_from_this()}, cb{std::move(cb)}](auto &&write_res) {
          self->is_writing_ = false;
          if (!write_res) {
            self->log_->error("write for stream {} failed: {}",
                              self->stream_id_.toString(),
                              write_res.error().message());
          }
          cb(std::forward<decltype(write_res)>(write_res));

          std::lock_guard<std::mutex> lock(self->write_queue_mutex_);
          // check if new write messages were received while stream was writing
          // and propagate these messages
          if (not self->write_queue_.empty()) {
            auto [in, bytes, cb] = self->write_queue_.front();
            self->write_queue_.pop_front();
            self->write(in, bytes, cb);
          }
        });
  }

  void MplexStream::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                              WriteCallbackFunc cb) {
    write(in, bytes, std::move(cb));
  }

  bool MplexStream::isClosed() const noexcept {
    return isClosedForRead() && isClosedForWrite();
  }

  void MplexStream::close(VoidResultHandlerFunc cb) {
    if (connection_.expired()) {
      return cb(Error::CONNECTION_IS_DEAD);
    }
    connection_.lock()->streamClose(
        stream_id_,
        [self{shared_from_this()}, cb{std::move(cb)}](auto &&close_res) {
          if (!close_res) {
            self->log_->error("cannot close stream {} for writes: {}",
                              self->stream_id_.toString(),
                              close_res.error().message());
            return cb(close_res.error());
          }

          self->is_writable_ = false;
          cb(outcome::success());
        });
  }

  bool MplexStream::isClosedForRead() const noexcept {
    return !is_readable_;
  }

  bool MplexStream::isClosedForWrite() const noexcept {
    return !is_writable_;
  }

  void MplexStream::reset() {
    auto conn = connection_.lock();
    if (!conn) {
      return;
    }

    is_reset_ = true;
    conn->streamReset(stream_id_);
  }

  void MplexStream::adjustWindowSize(uint32_t new_size,
                                     VoidResultHandlerFunc cb) {
    if (new_size == 0) {
      return cb(Error::BAD_WINDOW_SIZE);
    }
    receive_window_size_ = new_size;
    cb(outcome::success());
  }

  outcome::result<peer::PeerId> MplexStream::remotePeerId() const {
    TRY_GET_CONNECTION(conn)
    return conn->remotePeer();
  }

  outcome::result<bool> MplexStream::isInitiator() const {
    TRY_GET_CONNECTION(conn)
    return conn->isInitiator();
  }

  outcome::result<multi::Multiaddress> MplexStream::localMultiaddr() const {
    TRY_GET_CONNECTION(conn)
    return conn->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> MplexStream::remoteMultiaddr() const {
    TRY_GET_CONNECTION(conn)
    return conn->remoteMultiaddr();
  }

  outcome::result<void> MplexStream::commitData(gsl::span<const uint8_t> data,
                                                size_t data_size) {
    if (data_size == 0) {
      is_reset_ = true;
      if (data_notifyee_ && !data_notified_) {
        data_notifyee_(Error::CONNECTION_IS_DEAD);
      }
      return Error::CONNECTION_IS_DEAD;
    }

    if (data_size > receive_window_size_) {
      // we have received more data, than we can handle
      reset();
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

    if (data_notifyee_ && !data_notified_) {
      data_notifyee_(data_size);
    }

    return outcome::success();
  }
}  // namespace libp2p::connection

size_t std::hash<libp2p::connection::MplexStream::StreamId>::operator()(
    const libp2p::connection::MplexStream::StreamId &id) const {
  size_t seed = 0;
  boost::hash_combine(
      seed,
      std::hash<libp2p::connection::MplexStream::StreamNumber>()(id.number));
  boost::hash_combine(seed, std::hash<bool>()(id.initiator));
  return seed;
}
