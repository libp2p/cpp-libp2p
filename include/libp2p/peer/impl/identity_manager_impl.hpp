/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/peer/key_repository.hpp>

namespace libp2p::peer {
  class IdentityManagerImpl : public IdentityManager {
   public:
    ~IdentityManagerImpl() override = default;

    IdentityManagerImpl(
        crypto::KeyPair keyPair,
        const std::shared_ptr<crypto::marshaller::KeyMarshaller> &marshaller);

    const peer::PeerId &getId() const override;

    const crypto::KeyPair &getKeyPair() const override;

   private:
    std::unique_ptr<peer::PeerId> id_;
    std::unique_ptr<crypto::KeyPair> keyPair_;
  };

}  // namespace libp2p::peer
