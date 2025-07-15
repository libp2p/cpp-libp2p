/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/echo/server_echo_session.hpp>

#include <boost/assert.hpp>
#include <qtils/bytestr.hpp>

#include <libp2p/basic/write.hpp>

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
        self->log_->error("cannot close the stream: {}", res.error());
      }
    });
  }

  void ServerEchoSession::doRead() {
    if (stream_->isClosedForRead()
        || (!repeat_infinitely_ && config_.max_server_repeats == 0)) {
      return stop();
    }

    stream_->readSome(
        buf_, [self{shared_from_this()}](outcome::result<size_t> rread) {
          self->onRead(rread);
        });
  }

  void ServerEchoSession::onRead(outcome::result<size_t> rread) {
    if (!rread) {
      log_->error("error happened during read: {}", rread.error());
      return stop();
    }

    static constexpr size_t kMsgSizeThreshold = 120;

    if (rread.value() < kMsgSizeThreshold) {
      log_->debug(
          "read message: {}",
          std::string{buf_.begin(),
                      // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
                      buf_.begin() + rread.value()});
    } else {
      log_->debug("read {} bytes", rread.value());
    }
    buf_.resize(rread.value());
    doWrite();
  }

  void ServerEchoSession::doWrite() {
    if (stream_->isClosedForWrite() or buf_.empty()) {
      return stop();
    }
    write(stream_,
          buf_,
          [self{shared_from_this()}](outcome::result<void> rwrite) {
            self->onWrite(rwrite);
          });
  }

  void ServerEchoSession::onWrite(outcome::result<void> rwrite) {
    if (!rwrite) {
      log_->error("error happened during write: {}", rwrite.error());
      return stop();
    }

    if (buf_.size() < 120) {
      log_->info("written message: {}", qtils::byte2str(buf_));
    } else {
      log_->info("written {} bytes", buf_.size());
    }

    if (!repeat_infinitely_) {
      --config_.max_server_repeats;
    }
    doRead();
  }
}  // namespace libp2p::protocol
