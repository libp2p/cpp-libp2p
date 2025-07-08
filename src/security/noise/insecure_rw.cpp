/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arpa/inet.h>

#include <libp2p/security/noise/insecure_rw.hpp>

#include <libp2p/basic/read.hpp>
#include <libp2p/basic/write.hpp>
#include <libp2p/common/byteutil.hpp>
#include <libp2p/common/outcome_macro.hpp>
#include <libp2p/security/noise/crypto/state.hpp>

namespace libp2p::security::noise {
  InsecureReadWriter::InsecureReadWriter(
      std::shared_ptr<connection::LayerConnection> connection,
      std::shared_ptr<Bytes> buffer)
      : connection_{std::move(connection)}, buffer_{std::move(buffer)} {}

  void InsecureReadWriter::read(basic::MessageReadWriter::ReadCallbackFunc cb) {
    buffer_->reserve(kMaxMsgLen);  // ensure buffer capacity
    auto read_cb = [cb{std::move(cb)}, self{shared_from_this()}](
                       outcome::result<void> result) mutable {
      IF_ERROR_CB_RETURN(result);
      uint16_t frame_len{
          ntohs(common::convert<uint16_t>(self->buffer_->data()))};  // NOLINT
      auto read_cb = [cb = std::move(cb), self](outcome::result<void> result) {
        IF_ERROR_CB_RETURN(result);
        cb(self->buffer_);
      };
      self->buffer_->resize(frame_len);
      libp2p::read(self->connection_, *self->buffer_, std::move(read_cb));
    };
    buffer_->resize(kLengthPrefixSize);
    libp2p::read(connection_, *buffer_, std::move(read_cb));
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
    auto write_cb = [self{shared_from_this()}, buffer, cb{std::move(cb)}](
                        outcome::result<void> result) {
      IF_ERROR_CB_RETURN(result);
      cb(buffer.size());
    };
    libp2p::write(connection_, outbuf_, std::move(write_cb));
  }
}  // namespace libp2p::security::noise
