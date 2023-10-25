/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/ed25519_provider.hpp>

#include <libp2p/common/types.hpp>

namespace libp2p::crypto::ed25519 {

  class Ed25519ProviderImpl : public Ed25519Provider {
   public:
    outcome::result<Keypair> generate() const override;

    outcome::result<PublicKey> derive(
        const PrivateKey &private_key) const override;

    outcome::result<Signature> sign(
        BytesIn message, const PrivateKey &private_key) const override;

    outcome::result<bool> verify(BytesIn message,
                                 const Signature &signature,
                                 const PublicKey &public_key) const override;
  };

}  // namespace libp2p::crypto::ed25519
