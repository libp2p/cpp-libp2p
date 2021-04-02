/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/message_read_writer_bigendian.hpp>

#include <vector>

#include <arpa/inet.h>
#include <boost/assert.hpp>
#include <libp2p/basic/message_read_writer_error.hpp>
#include <libp2p/common/byteutil.hpp>

namespace libp2p::basic {
  MessageReadWriterBigEndian::MessageReadWriterBigEndian(
      std::shared_ptr<ReadWriter> conn)
      : conn_{std::move(conn)} {
    BOOST_ASSERT(conn_ != nullptr);
  }

  void MessageReadWriterBigEndian::read(ReadCallbackFunc cb) {
    auto buffer = std::make_shared<std::vector<uint8_t>>();
    buffer->resize(kLenMarkerSize);
    conn_->read(
        *buffer, kLenMarkerSize,
        [self{shared_from_this()}, buffer, cb{std::move(cb)}](auto &&result) {
          if (not result) {
            return cb(result.error());
          }
          uint32_t msg_len = ntohl(  // NOLINT
              common::convert<uint32_t>(buffer->data()));
          buffer->resize(msg_len);
          std::fill(buffer->begin(), buffer->end(), 0u);
          self->conn_->read(*buffer, msg_len,
                            [self, buffer, cb](auto &&result) {
                              if (not result) {
                                return cb(result.error());
                              }
                              cb(buffer);
                            });
        });
  }

  void MessageReadWriterBigEndian::write(gsl::span<const uint8_t> buffer,
                                         Writer::WriteCallbackFunc cb) {
    if (buffer.empty()) {
      // TODO(107): Reentrancy
      return cb(MessageReadWriterError::BUFFER_IS_EMPTY);
    }

    std::vector<uint8_t> raw_buf;
    raw_buf.reserve(kLenMarkerSize + buffer.size());
    common::putUint32BE(raw_buf, buffer.size());
    raw_buf.insert(raw_buf.end(), buffer.begin(), buffer.end());
    conn_->write(raw_buf, raw_buf.size(),
                 [self{shared_from_this()}, cb{std::move(cb)}](auto &&result) {
                   if (not result) {
                     return cb(result.error());
                   }
                   cb(result.value() - self->kLenMarkerSize);
                 });
  }
}  // namespace libp2p::basic
