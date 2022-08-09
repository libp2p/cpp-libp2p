/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/mplex/mplex_stream.hpp>

#include <algorithm>

#include <boost/container_hash/hash.hpp>
#include <libp2p/muxer/mplex/mplexed_connection.hpp>

#define TRY_GET_CONNECTION(conn_var_name) \
  if (connection_.expired()) {            \
    return Error::STREAM_RESET_BY_HOST;   \
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

  void MplexStream::readDone(outcome::result<size_t> res) {
    auto cb{std::move(reading_->cb)};
    reading_.reset();
    cb(res);
  }

  bool MplexStream::readTry() {
    auto size{std::min(read_buffer_.size(), reading_->bytes)};
    if (reading_->some) {
      if (size == 0) {
        return false;
      }
    } else {
      if (size != reading_->bytes) {
        return false;
      }
    }
    if (boost::asio::buffer_copy(
            boost::asio::buffer(reading_->out.data(), size),
            read_buffer_.data(), size)
        != size) {
      readDone(Error::STREAM_INTERNAL_ERROR);
      return true;
    }
    read_buffer_.consume(size);
    receive_window_size_ += size;
    readDone(size);
    return true;
  }

  void MplexStream::read(gsl::span<uint8_t> out, size_t bytes,
                         ReadCallbackFunc cb, bool some) {
    if (is_reset_) {
      return cb(Error::STREAM_RESET_BY_PEER);
    }
    if (!is_readable_) {
      return cb(Error::STREAM_NOT_READABLE);
    }
    if (bytes == 0 || out.empty() || static_cast<size_t>(out.size()) < bytes) {
      return cb(Error::STREAM_INVALID_ARGUMENT);
    }
    if (reading_.has_value()) {
      return cb(Error::STREAM_IS_READING);
    }

    reading_.emplace(Reading{out, bytes, std::move(cb), some});
    if (readTry()) {
      return;
    }

    if (connection_.expired()) {
      return readDone(Error::STREAM_RESET_BY_HOST);
    }
  }

  void MplexStream::write(gsl::span<const uint8_t> in, size_t bytes,
                          WriteCallbackFunc cb) {
    // TODO(107): Reentrancy

    if (is_reset_) {
      return cb(Error::STREAM_RESET_BY_PEER);
    }
    if (!is_writable_) {
      return cb(Error::STREAM_NOT_WRITABLE);
    }
    if (bytes == 0 || in.empty() || static_cast<size_t>(in.size()) < bytes) {
      return cb(Error::STREAM_INVALID_ARGUMENT);
    }
    if (is_writing_) {
      std::vector<uint8_t> in_vector(in.begin(), in.end());
      std::lock_guard<std::mutex> lock(write_queue_mutex_);
      write_queue_.emplace_back(in_vector, bytes, cb);
      return;
    }
    if (connection_.expired()) {
      return cb(Error::STREAM_RESET_BY_HOST);
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

  void MplexStream::deferReadCallback(outcome::result<size_t> res,
                                      ReadCallbackFunc cb) {
    if (connection_.expired()) {
      // TODO(107) Reentrancy here, defer callback
      return cb(Error::STREAM_RESET_BY_HOST);
    }
    connection_.lock()->deferReadCallback(res, std::move(cb));
  }

  void MplexStream::deferWriteCallback(std::error_code ec,
                                       WriteCallbackFunc cb) {
    if (connection_.expired()) {
      // TODO(107) Reentrancy here, defer callback
      return cb(Error::STREAM_RESET_BY_HOST);
    }
    connection_.lock()->deferWriteCallback(ec, std::move(cb));
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
      return cb(Error::STREAM_RESET_BY_HOST);
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
      return cb(Error::STREAM_INVALID_WINDOW_SIZE);
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
      if (reading_.has_value()) {
        readDone(Error::STREAM_RESET_BY_HOST);
      }
      return Error::STREAM_RESET_BY_HOST;
    }

    if (data_size > receive_window_size_) {
      // we have received more data, than we can handle
      reset();
      return Error::STREAM_RECEIVE_OVERFLOW;
    }

    if (boost::asio::buffer_copy(
            read_buffer_.prepare(data_size),
            boost::asio::const_buffer(data.data(), data_size))
        != data_size) {
      return Error::STREAM_INTERNAL_ERROR;
    }
    read_buffer_.commit(data_size);
    receive_window_size_ -= data_size;

    if (reading_.has_value()) {
      readTry();
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
