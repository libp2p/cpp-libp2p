/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_SHA256_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_SHA256_HPP

#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {

  class NoiseSHA256HasherImpl : public NamedHasher {
   public:
    std::shared_ptr<libp2p::crypto::Hasher> hash() override;

    std::string hashName() const override;
  };

}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_SHA256_HPP
