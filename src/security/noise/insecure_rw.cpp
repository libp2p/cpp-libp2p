/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arpa/inet.h>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <libp2p/security/noise/insecure_rw.hpp>

#include <libp2p/basic/write_return_size.hpp>
#include <libp2p/common/byteutil.hpp>
#include <libp2p/security/noise/crypto/state.hpp>

#ifndef UNIQUE_NAME
#define UNIQUE_NAME(base) base##__LINE__
#endif  // UNIQUE_NAME

#define IO_OUTCOME_TRY_NAME(var, val, res, cb) \
  auto && (var) = (res);                       \
  if ((var).has_error()) {                     \
    cb((var).error());                         \
    return;                                    \
  }                                            \
  auto && (val) = (var).value();

#define IO_OUTCOME_TRY(name, res, cb) \
  IO_OUTCOME_TRY_NAME(UNIQUE_NAME(name), name, res, cb)

namespace libp2p::security::noise {
  InsecureReadWriter::InsecureReadWriter(
      std::shared_ptr<connection::LayerConnection> connection,
      std::shared_ptr<Bytes> buffer)
      : connection_{std::move(connection)}, buffer_{std::move(buffer)} {}

  void InsecureReadWriter::read(basic::MessageReadWriter::ReadCallbackFunc cb) {
    buffer_->resize(kMaxMsgLen);  // ensure buffer capacity
    auto read_cb = [cb{std::move(cb)}, self{shared_from_this()}](
                       outcome::result<size_t> result) mutable {
      IO_OUTCOME_TRY(read_bytes, result, cb);
      if (kLengthPrefixSize != read_bytes) {
        return cb(std::errc::broken_pipe);
      }
      uint16_t frame_len{
          ntohs(common::convert<uint16_t>(self->buffer_->data()))};  // NOLINT
      auto read_cb = [cb = std::move(cb), self, frame_len](
                         outcome::result<size_t> result) {
        IO_OUTCOME_TRY(read_bytes, result, cb);
        if (frame_len != read_bytes) {
          return cb(std::errc::broken_pipe);
        }
        self->buffer_->resize(read_bytes);
        cb(self->buffer_);
      };
      self->connection_->read(*self->buffer_, frame_len, std::move(read_cb));
    };
    connection_->read(*buffer_, kLengthPrefixSize, std::move(read_cb));
  }

  void InsecureReadWriter::write(BytesIn buffer,
                                 basic::Writer::WriteCallbackFunc cb) {
    if (buffer.size() > static_cast<int64_t>(kMaxMsgLen)) {
      return cb(std::errc::message_size);
    }
    outbuf_.clear();
    outbuf_.reserve(kLengthPrefixSize + buffer.size());
    common::putUint16BE(outbuf_, buffer.size());
    outbuf_.insert(outbuf_.end(), buffer.begin(), buffer.end());
    auto write_cb = [self{shared_from_this()},
                     cb{std::move(cb)}](outcome::result<size_t> result) {
      IO_OUTCOME_TRY(written_bytes, result, cb);
      if (self->outbuf_.size() != written_bytes) {
        return cb(std::errc::broken_pipe);
      }
      cb(written_bytes - kLengthPrefixSize);
    };
    writeReturnSize(connection_, outbuf_, std::move(write_cb));
  }

  boost::asio::awaitable<basic::MessageReadWriter::ReadCallback>
  InsecureReadWriter::read() {
    buffer_->resize(kMaxMsgLen);  // ensure buffer capacity

    // Read the length prefix
    auto read_length_result = co_await connection_->read(*buffer_, kLengthPrefixSize);
    if (!read_length_result) {
      co_return read_length_result.error();
    }

    if (kLengthPrefixSize != read_length_result.value()) {
      co_return std::errc::broken_pipe;
    }

    // Extract the frame length from the prefix
    uint16_t frame_len = ntohs(common::convert<uint16_t>(buffer_->data()));  // NOLINT

    // Read the actual data
    auto read_data_result = co_await connection_->read(*buffer_, frame_len);
    if (!read_data_result) {
      co_return read_data_result.error();
    }

    if (frame_len != read_data_result.value()) {
      co_return std::errc::broken_pipe;
    }

    // Resize buffer to actual data size and return it
    buffer_->resize(read_data_result.value());
    co_return buffer_;
  }

  boost::asio::awaitable<outcome::result<size_t>>
  InsecureReadWriter::write(BytesIn buffer) {
    if (buffer.size() > static_cast<int64_t>(kMaxMsgLen)) {
      co_return std::errc::message_size;
    }

    // Prepare the output buffer with length prefix
    outbuf_.clear();
    outbuf_.reserve(kLengthPrefixSize + buffer.size());
    common::putUint16BE(outbuf_, buffer.size());
    outbuf_.insert(outbuf_.end(), buffer.begin(), buffer.end());

    // Use a promise to handle the completion
    struct ContextState {
      std::optional<outcome::result<size_t>> result;
      bool done = false;
    };

    auto state = std::make_shared<ContextState>();

    writeReturnSize(connection_, outbuf_, [state](auto result) {
      state->result = result;
      state->done = true;
    });

    // Wait until write operation completes
    while (!state->done) {
      co_await boost::asio::post(boost::asio::use_awaitable);
    }

    if (!state->result->has_value()) {
      co_return state->result->error();
    }

    if (outbuf_.size() != state->result->value()) {
      co_return std::errc::broken_pipe;
    }

    // Return number of actual payload bytes written (excluding length prefix)
    co_return state->result->value() - kLengthPrefixSize;
  }
}  // namespace libp2p::security::noise
