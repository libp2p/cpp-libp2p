/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_DH_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_DH_HPP

#include <libp2p/crypto/x25519_provider/x25519_provider_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {

  /// X25519 DH
  class NoiseDiffieHellmanImpl : public DiffieHellman {
   public:
    outcome::result<DHKey> generate() override;

    outcome::result<ByteArray> dh(const ByteArray &private_key,
                                  const ByteArray &public_key) override;

    int dhSize() const override;

    std::string dhName() const override;

   private:
    crypto::x25519::X25519ProviderImpl x25519;
  };

}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_DH_HPP
