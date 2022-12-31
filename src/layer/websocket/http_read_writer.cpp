/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/http_read_writer.hpp>

#include <arpa/inet.h>

#include <libp2p/common/byteutil.hpp>

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

namespace libp2p::layer::websocket {
  HttpReadWriter::HttpReadWriter(
      std::shared_ptr<connection::LayerConnection> connection,
      std::shared_ptr<common::ByteArray> buffer)
      : connection_{std::move(connection)}, buffer_{std::move(buffer)} {}

  void HttpReadWriter::read(basic::MessageReadWriter::ReadCallbackFunc cb) {
    buffer_->resize(kMaxMsgLen);  // ensure buffer capacity
    auto read_cb = [cb{std::move(cb)}, self{shared_from_this()}](
                       outcome::result<size_t> result) mutable {
      IO_OUTCOME_TRY(read_bytes, result, cb);
      if (read_bytes == 0) {
        return cb(std::errc::broken_pipe);
      }
      size_t i;
      bool headers_read = false;
      int headers_end = 0;
      if (read_bytes >= 4) {
        gsl::span<const uint8_t> data(self->buffer_->data(), read_bytes);
        gsl::span<const uint8_t, 4> delimiter(
            reinterpret_cast<const uint8_t *>("\r\n\r\n"), 4);  // NOLINT

        for (i = 0; i <= read_bytes - delimiter.size(); ++i) {
          if (data.subspan(i, delimiter.size()) == delimiter) {
            headers_read = true;
            headers_end = i + delimiter.size();
            break;
          }
        }
      }

      if (headers_read) {
        auto headers_raw = std::make_shared<common::ByteArray>(
            self->buffer_->begin(),
            std::next(self->buffer_->begin(), headers_end));
        cb(headers_raw);
        return;
      }

      // uint16_t frame_len{
      //     ntohs(common::convert<uint16_t>())}; // NOLINT
      // auto read_cb = [cb = std::move(cb), self,
      //                 frame_len](outcome::result<size_t> result) {
      //   IO_OUTCOME_TRY(read_bytes, result, cb);
      //   if (frame_len != read_bytes) {
      //     return cb(std::errc::broken_pipe);
      //   }
      //   self->buffer_->resize(read_bytes);
      //   cb(self->buffer_);
      // };
      //
      // self->connection_->read(*self->buffer_, kMaxMsgLen,
      // std::move(read_cb));
      return;
    };
    connection_->readSome(*buffer_, kMaxMsgLen, std::move(read_cb));
  }

  void HttpReadWriter::write(gsl::span<const uint8_t> buffer,
                             basic::Writer::WriteCallbackFunc cb) {
    if (buffer.size() > static_cast<int64_t>(kMaxMsgLen)) {
      return cb(std::errc::message_size);
    }

    outbuf_.clear();
    outbuf_.reserve(buffer.size());
    outbuf_.insert(outbuf_.end(), buffer.begin(), buffer.end());

    auto write_cb = [self{shared_from_this()},
                     cb{std::move(cb)}](outcome::result<size_t> result) {
      IO_OUTCOME_TRY(written_bytes, result, cb);
      if (self->outbuf_.size() != written_bytes) {
        return cb(std::errc::broken_pipe);
      }
      cb(written_bytes);
    };

    connection_->write(outbuf_, outbuf_.size(), std::move(write_cb));
  }
}  // namespace libp2p::layer::websocket
