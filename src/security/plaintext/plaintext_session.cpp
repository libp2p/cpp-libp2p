/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/plaintext/plaintext_session.hpp>

namespace libp2p::security {

  PlaintextSession::PlaintextSession(
      std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller,
      std::shared_ptr<connection::RawConnection> conn,
      SecurityAdaptor::SecConnCallbackFunc handler)
      : marshaller_(std::move(marshaller)),
        conn_(std::move(conn)),
        handler_(std::move(handler)),
        // key will not be larger than 10KB
        recvbuf_(10000, 0) {
    BOOST_ASSERT(marshaller_ != nullptr);
    BOOST_ASSERT(conn_ != nullptr);
    BOOST_ASSERT(handler_ != nullptr);
  }

  void PlaintextSession::recvKey(PlaintextSession::PubkeyFunc f) {
    conn_->readSome(
        recvbuf_,
        recvbuf_.size(),
        [self{shared_from_this()}, f{std::move(f)}](outcome::result<size_t> r) {
          if (!r) {
            return self->handler_(r.error());
          }

          self->recvbuf_.resize(r.value());

          auto rpub = self->marshaller_->unmarshalPublicKey(
              crypto::ProtobufKey{self->recvbuf_});
          if (!rpub) {
            return self->handler_(rpub.error());
          }

          return f(rpub.value());
        });
  }

  void PlaintextSession::sendKey(const crypto::PublicKey &publicKey,
                                 PlaintextSession::ThenFunc then) {
    auto r = marshaller_->marshal(publicKey);
    if (!r) {
      return handler_(r.error());
    }
    sendbuf_ = r.value().key;

    conn_->write(sendbuf_,
                 sendbuf_.size(),
                 [self{this->shared_from_this()},
                  then{std::move(then)}](outcome::result<size_t> r) {
                   if (!r) {
                     return self->handler_(r.error());
                   }

                   return then();
                 });
  }
}  // namespace libp2p::security
