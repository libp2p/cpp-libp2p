/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/echo/client_echo_session.hpp>

#include <boost/assert.hpp>

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

    auto self{shared_from_this()};
    stream_->write(
        buf_, buf_.size(),
        [self, then{std::move(then)}](outcome::result<size_t> rw) mutable {
          if (!rw) {
            return then(rw.error());
          }

          if (self->stream_->isClosedForRead()) {
            return;
          }

          self->stream_->read(
              self->buf_, self->buf_.size(),
              [self,
               then{std::move(then)}](outcome::result<size_t> rr) mutable {
                if (!rr) {
                  return then(rr.error());
                }

                auto begin = self->buf_.begin();
                return then(std::string(begin, begin + rr.value()));
              });
        });
  }
}  // namespace libp2p::protocol
