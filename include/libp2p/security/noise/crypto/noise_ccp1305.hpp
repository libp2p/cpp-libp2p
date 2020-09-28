/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_CCP1305_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_CCP1305_HPP

#include <libp2p/crypto/chachapoly/chachapoly_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {
  class CCP1305Impl : public AEADCipher {
   public:
    ~CCP1305Impl() override = default;

    outcome::result<ByteArray> encrypt(uint64_t nonce,
                                       gsl::span<const uint8_t> plaintext,
                                       gsl::span<const uint8_t> aad) override;

    outcome::result<ByteArray> decrypt(uint64_t nonce,
                                       gsl::span<const uint8_t> ciphertext,
                                       gsl::span<const uint8_t> aad) override;

  };
}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_CCP1305_HPP
