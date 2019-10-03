/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/echo/server_echo_session.hpp>

#include <boost/assert.hpp>

namespace libp2p::protocol {

  ServerEchoSession::ServerEchoSession(
      std::shared_ptr<connection::Stream> stream, EchoConfig config)
      : stream_(std::move(stream)),
        buf_(config.max_recv_size, 0),
        config_{config},
        repeat_infinitely_{config.max_server_repeats == 0} {
    BOOST_ASSERT(stream_ != nullptr);
    BOOST_ASSERT(config_.max_recv_size > 0);
  }

  void ServerEchoSession::start() {
    doRead();
  }

  void ServerEchoSession::stop() {
    stream_->close([self{shared_from_this()}](auto &&res) {
      if (!res) {
        self->log_->error("cannot close the stream: {}", res.error().message());
      }
    });
  }

  void ServerEchoSession::doRead() {
    if (stream_->isClosedForRead()
        || (!repeat_infinitely_ && config_.max_server_repeats == 0)) {
      return stop();
    }

    stream_->readSome(
        buf_, buf_.size(),
        [self{shared_from_this()}](outcome::result<size_t> rread) {
          self->onRead(rread);
        });
  }

  void ServerEchoSession::onRead(outcome::result<size_t> rread) {
    if (!rread) {
      log_->error("error happened during read: {}", rread.error().message());
      return stop();
    }

    log_->info("read message: {}",
               std::string{buf_.begin(), buf_.begin() + rread.value()});
    this->doWrite(rread.value());
  }

  void ServerEchoSession::doWrite(size_t size) {
    if (stream_->isClosedForWrite()) {
      return stop();
    }

    stream_->write(buf_, size,
                   [self{shared_from_this()}](outcome::result<size_t> rwrite) {
                     self->onWrite(rwrite);
                   });
  }

  void ServerEchoSession::onWrite(outcome::result<size_t> rwrite) {
    if (!rwrite) {
      log_->error("error happened during write: {}", rwrite.error().message());
      return stop();
    }

    log_->info("written message: {}",
               std::string{buf_.begin(), buf_.begin() + rwrite.value()});

    if (!repeat_infinitely_) {
      --config_.max_server_repeats;
    }
    doRead();
  }
}  // namespace libp2p::protocol
