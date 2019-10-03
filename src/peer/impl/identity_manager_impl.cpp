/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/impl/identity_manager_impl.hpp>

#include "libp2p/crypto/key_marshaller.hpp"

namespace libp2p::peer {

  const peer::PeerId &IdentityManagerImpl::getId() const {
    BOOST_ASSERT(id_ != nullptr);
    return *id_;
  }

  const crypto::KeyPair &IdentityManagerImpl::getKeyPair() const {
    BOOST_ASSERT(keyPair_ != nullptr);
    return *keyPair_;
  }

  IdentityManagerImpl::IdentityManagerImpl(
      crypto::KeyPair keyPair,
      const std::shared_ptr<crypto::marshaller::KeyMarshaller> &marshaller) {
    BOOST_ASSERT(!keyPair.publicKey.data.empty());
    BOOST_ASSERT(marshaller);

    keyPair_ = std::make_unique<crypto::KeyPair>(std::move(keyPair));

    // it is ok to use .value()
    auto id = peer::PeerId::fromPublicKey(
                  marshaller->marshal(keyPair_->publicKey).value())
                  .value();
    id_ = std::make_unique<peer::PeerId>(std::move(id));
  }
}  // namespace libp2p::peer
