/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/x25519_provider/x25519_provider_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {

  /// X25519 DH
  class NoiseDiffieHellmanImpl : public DiffieHellman {
   public:
    outcome::result<DHKey> generate() override;

    outcome::result<Bytes> dh(const Bytes &private_key,
                              const Bytes &public_key) override;

    int dhSize() const override;

    std::string dhName() const override;

   private:
    crypto::x25519::X25519ProviderImpl x25519;
  };

}  // namespace libp2p::security::noise
