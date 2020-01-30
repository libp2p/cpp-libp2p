/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_ED25519_PROVIDER_ED25519_PROVIDER_IMPL_HPP
#define LIBP2P_ED25519_PROVIDER_ED25519_PROVIDER_IMPL_HPP

#include <libp2p/crypto/ed25519_provider.hpp>

namespace libp2p::crypto::ed25519 {

  class Ed25519ProviderImpl : public Ed25519Provider {
   public:
    outcome::result<Keypair> generate() const override;

    outcome::result<PublicKey> derive(
        const PrivateKey &private_key) const override;

    outcome::result<Signature> sign(
        gsl::span<const uint8_t> message,
        const PrivateKey &private_key) const override;

    outcome::result<bool> verify(gsl::span<const uint8_t> message,
                                 const Signature &signature,
                                 const PublicKey &public_key) const override;
  };

}  // namespace libp2p::crypto::ed25519

#endif  // LIBP2P_ED25519_PROVIDER_ED25519_PROVIDER_IMPL_HPP
