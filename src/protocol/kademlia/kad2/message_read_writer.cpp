/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_read_writer.hpp"

namespace libp2p::kad2 {

  MessageReadWriter::MessageReadWriter(
    basic::ReadWriter& conn,
    MessageReadWriter::ReadResultFn rr, MessageReadWriter::WriteResultFn wr
  ) :
    log_(common::createLogger("kad")),
    mrw_(conn),
    read_cb_(std::move(rr)),
    write_cb_(std::move(wr))
  {}

  void MessageReadWriter::read() {
    mrw_->read([this](basic::MessageReadWriter::ReadCallback result) {
      onRead(std::move(result));
    });
  }

  void MessageReadWriter::onRead(basic::MessageReadWriter::ReadCallback result) {
    if (!read_cb_) return;
    if (!result) {
      read_cb_(outcome::failure(result.error()));
    }
    Message msg;
    auto& v = *result.value();
    if (!msg.deserialize(v.data(), v.size())) {
      read_cb_(outcome::failure(Error::MESSAGE_PARSE_ERROR));
    }
    read_cb_(std::move(msg));
  }

  void MessageReadWriter::write(const Message& msg) {
    if (!msg.serialize(buffer_)) {
      if (write_cb_) write_cb_(outcome::failure(Error::MESSAGE_SERIALIZE_ERROR));
    }
    mrw_->write(gsl::span(buffer_.data(), buffer_.size()),
      [this](outcome::result<size_t> result) {
        if (write_cb_) write_cb_(result);
      }
    );
  }

}  // namespace libp2p::protocol::kademlia
