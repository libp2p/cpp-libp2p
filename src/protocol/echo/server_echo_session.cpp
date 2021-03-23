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
        config_{config},
        repeat_infinitely_{config.max_server_repeats == 0} {
    BOOST_ASSERT(stream_ != nullptr);
    BOOST_ASSERT(config_.max_recv_size > 0);

    size_t max_recv_size = 65536;
    if (config_.max_recv_size < max_recv_size) {
      max_recv_size = config_.max_recv_size;
    }
    buf_.resize(max_recv_size);
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

    static constexpr size_t kMsgSizeThreshold = 120;

    if (rread.value() < kMsgSizeThreshold) {
      log_->debug("read message: {}",
                 std::string{buf_.begin(), buf_.begin() + rread.value()});
    } else {
      log_->debug("read {} bytes", rread.value());
    }
    this->doWrite(rread.value());
    doRead();
  }

  void ServerEchoSession::doWrite(size_t size) {
    if (stream_->isClosedForWrite() || size == 0) {
      return stop();
    }

    auto write_buf = std::vector<uint8_t>(buf_.begin(), buf_.begin() + size);
    gsl::span<const uint8_t> span = write_buf;
    stream_->write(
        span, size,
        [self{shared_from_this()}, write_buf{std::move(write_buf)}](
            outcome::result<size_t> rwrite) { self->onWrite(rwrite); });
  }

  void ServerEchoSession::onWrite(outcome::result<size_t> rwrite) {
    if (!rwrite) {
      log_->error("error happened during write: {}", rwrite.error().message());
      return stop();
    }

    if (rwrite.value() < 120) {
      log_->info("written message: {}",
                 std::string{buf_.begin(), buf_.begin() + rwrite.value()});
    } else {
      log_->info("written {} bytes", rwrite.value());
    }

    if (!repeat_infinitely_) {
      --config_.max_server_repeats;
    }
  }
}  // namespace libp2p::protocol
