/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/ping/ping_server_session.hpp>

#include <boost/assert.hpp>
#include <libp2p/protocol/ping/common.hpp>

namespace libp2p::protocol {
  PingServerSession::PingServerSession(
      std::shared_ptr<connection::Stream> stream, PingConfig config)
      : stream_{std::move(stream)},
        config_{config},
        buffer_(config_.message_size, 0) {
    BOOST_ASSERT(stream_);
  }

  void PingServerSession::start() {
    BOOST_ASSERT(!is_started_);
    is_started_ = true;

    read();
  }

  void PingServerSession::read() {
    if (!is_started_ || stream_->isClosedForRead()) {
      return;
    }

    stream_->read(buffer_, config_.message_size,
                  [self{shared_from_this()}](auto &&read_res) {
                    if (!read_res) {
                      return;
                    }
                    self->readCompleted();
                  });
  }

  void PingServerSession::readCompleted() {
    write();
  }

  void PingServerSession::write() {
    if (!is_started_ || stream_->isClosedForWrite()) {
      return;
    }

    stream_->write(buffer_, config_.message_size,
                   [self{shared_from_this()}](auto &&write_res) {
                     if (!write_res) {
                       return;
                     }
                     self->writeCompleted();
                   });
  }

  void PingServerSession::writeCompleted() {
    read();
  }
}  // namespace libp2p::protocol
