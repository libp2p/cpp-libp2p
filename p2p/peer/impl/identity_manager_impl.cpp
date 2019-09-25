/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "peer/impl/identity_manager_impl.hpp"

namespace libp2p::peer {

  const peer::PeerId &IdentityManagerImpl::getId() const {
    BOOST_ASSERT(id_ != nullptr);
    return *id_;
  }

  const crypto::KeyPair &IdentityManagerImpl::getKeyPair() const {
    BOOST_ASSERT(keyPair_ != nullptr);
    return *keyPair_;
  }

  IdentityManagerImpl::IdentityManagerImpl(crypto::KeyPair keyPair) {
    BOOST_ASSERT(!keyPair.publicKey.data.empty());

    keyPair_ = std::make_unique<crypto::KeyPair>(std::move(keyPair));

    // it is ok to use .value()
    auto id = peer::PeerId::fromPublicKey(keyPair_->publicKey);
    id_ = std::make_unique<peer::PeerId>(std::move(id));
  }
}  // namespace libp2p::peer
