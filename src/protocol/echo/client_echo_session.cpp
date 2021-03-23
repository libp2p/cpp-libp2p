/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/echo/client_echo_session.hpp>

#include <boost/assert.hpp>

#include <libp2p/log/logger.hpp>

namespace libp2p::protocol {

  ClientEchoSession::ClientEchoSession(
      std::shared_ptr<connection::Stream> stream)
      : stream_(std::move(stream)) {
    BOOST_ASSERT(stream_ != nullptr);
  }

  void ClientEchoSession::sendAnd(const std::string &send,
                                  ClientEchoSession::Then then) {
    if (stream_->isClosedForWrite()) {
      return;
    }

    buf_ = std::vector<uint8_t>(send.begin(), send.end());
    recv_buf_.resize(buf_.size());
    ec_.clear();
    bytes_read_ = 0;
    then_ = std::move(then);

    auto self{shared_from_this()};

    stream_->write(buf_, buf_.size(), [self](outcome::result<size_t> rw) {
      if (!rw && !self->ec_) {
        self->ec_ = rw.error();
        self->completed();
      }
    });

    doRead();
  }

  void ClientEchoSession::doRead() {
    auto self{shared_from_this()};

    gsl::span<uint8_t> span = recv_buf_;
    span = span.subspan(bytes_read_);

    if (span.empty()) {
      completed();
    }

    stream_->readSome(span, span.size(),
                      [self](outcome::result<size_t> rr) {
                        if (!rr && !self->ec_) {
                          self->ec_ = rr.error();
                          return self->completed();
                        }

                        if (rr) {
                          self->bytes_read_ += rr.value();
                          return self->doRead();
                        }
                      });
  }

  void ClientEchoSession::completed() {
    if (then_) {
      auto then = decltype(then_){};
      then_.swap(then);
      if (ec_) {
        then(ec_);
      } else {
        if (recv_buf_ != buf_) {
          log::createLogger("Echo")->error(
              "ClientEchoSession: send and receive buffers mismatch");
        }
        auto begin = recv_buf_.begin();
        then(std::string(begin, begin + recv_buf_.size()));
      }
    }
  };

}  // namespace libp2p::protocol
