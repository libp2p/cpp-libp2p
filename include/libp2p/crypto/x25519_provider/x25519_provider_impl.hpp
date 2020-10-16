/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_CRYPTO_X25519_PROVIDER_X25519_PROVIDER_IMPL_HPP
#define LIBP2P_INCLUDE_LIBP2P_CRYPTO_X25519_PROVIDER_X25519_PROVIDER_IMPL_HPP

#include <libp2p/crypto/x25519_provider.hpp>

namespace libp2p::crypto::x25519 {

  class X25519ProviderImpl : public X25519Provider {
   public:
    outcome::result<Keypair> generate() const override;

    outcome::result<PublicKey> derive(
        const PrivateKey &private_key) const override;

    outcome::result<std::vector<uint8_t>> dh(
        const PrivateKey &private_key,
        const PublicKey &public_key) const override;
  };

}  // namespace libp2p::crypto::x25519

#endif  // LIBP2P_INCLUDE_LIBP2P_CRYPTO_X25519_PROVIDER_X25519_PROVIDER_IMPL_HPP
