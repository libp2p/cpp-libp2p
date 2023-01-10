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

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::layer::websocket, HttpReadWriter::Error,
                            e) {
  using E = libp2p::layer::websocket::HttpReadWriter::Error;
  switch (e) {
    case E::BAD_REQUEST_BAD_HEADERS:
      return "Invalid headers or header is too long";
    default:
      return "Unknown error (HttpReadWriter::Error)";
  }
}

namespace libp2p::layer::websocket {
  HttpReadWriter::HttpReadWriter(
      std::shared_ptr<connection::LayerConnection> connection,
      std::shared_ptr<common::ByteArray> buffer)
      : connection_{std::move(connection)}, read_buffer_{std::move(buffer)} {}

  void HttpReadWriter::read(basic::MessageReadWriter::ReadCallbackFunc cb) {
    read_buffer_->resize(kMaxMsgLen);  // ensure buffer capacity
    auto read_cb = [cb{std::move(cb)}, self{shared_from_this()}](
                       outcome::result<size_t> result) mutable {
      IO_OUTCOME_TRY(read_bytes, result, cb);
      if (read_bytes == 0) {
        return cb(std::errc::broken_pipe);
      }
      bool headers_read = false;
      size_t headers_end = 0;
      if (read_bytes >= 4) {
        gsl::span<const uint8_t> data(self->read_buffer_->data(), read_bytes);
        gsl::span<const uint8_t, 4> delimiter(
            reinterpret_cast<const uint8_t *>("\r\n\r\n"), 4);  // NOLINT

        auto start_pos = self->read_bytes_ > delimiter.size()
            ? self->read_bytes_ - delimiter.size()
            : 0;
        for (size_t i = start_pos; i <= read_bytes - delimiter.size(); ++i) {
          if (data.subspan(i, delimiter.size()) == delimiter) {
            headers_read = true;
            headers_end = i + delimiter.size();
            break;
          }
        }
      }
      self->read_bytes_ += read_bytes;

      if (headers_read) {
        auto headers_raw = std::make_shared<common::ByteArray>(
            self->read_buffer_->begin(),
            std::next(self->read_buffer_->begin(), headers_end));
        self->processed_bytes_ = headers_raw->size();
        cb(headers_raw);
        return;
      }

      if (headers_end == self->read_buffer_->size()) {
        cb(Error::BAD_REQUEST_BAD_HEADERS);
        return;
      }

      self->read(std::move(cb));
      return;
    };

    connection_->readSome(
        gsl::make_span(std::next(read_buffer_->data(), read_bytes_),
                       read_buffer_->size() - read_bytes_),
        kMaxMsgLen, std::move(read_cb));
  }

  void HttpReadWriter::write(gsl::span<const uint8_t> buffer,
                             basic::Writer::WriteCallbackFunc cb) {
    if (buffer.size() > static_cast<int64_t>(kMaxMsgLen)) {
      return cb(std::errc::message_size);
    }

    send_buffer_.clear();
    send_buffer_.reserve(buffer.size());
    send_buffer_.insert(send_buffer_.end(), buffer.begin(), buffer.end());

    auto write_cb = [self{shared_from_this()},
                     cb{std::move(cb)}](outcome::result<size_t> result) {
      IO_OUTCOME_TRY(written_bytes, result, cb);
      if (self->send_buffer_.size() != written_bytes) {
        return cb(std::errc::broken_pipe);
      }
      cb(written_bytes);
    };

    connection_->write(send_buffer_, send_buffer_.size(), std::move(write_cb));
  }
}  // namespace libp2p::layer::websocket
