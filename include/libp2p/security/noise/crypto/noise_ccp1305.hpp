/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_CCP1305_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_CCP1305_HPP

#include <libp2p/crypto/chachapoly/chachapoly_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {
  class NoiseCCP1305Impl : public AEADCipher {
   public:
    NoiseCCP1305Impl(Key32 key);

    outcome::result<ByteArray> encrypt(gsl::span<const uint8_t> precompiled_out,
                                       uint64_t nonce,
                                       gsl::span<const uint8_t> plaintext,
                                       gsl::span<const uint8_t> aad) override;

    outcome::result<ByteArray> decrypt(gsl::span<const uint8_t> precompiled_out,
                                       uint64_t nonce,
                                       gsl::span<const uint8_t> ciphertext,
                                       gsl::span<const uint8_t> aad) override;

   private:
    std::unique_ptr<crypto::chachapoly::ChaCha20Poly1305> ccp_;
  };

  class NamedCCPImpl : public NamedAEADCipher {
   public:
    ~NamedCCPImpl() override = default;

    std::shared_ptr<AEADCipher> cipher(Key32 key) override;

    std::string cipherName() const override;
  };
}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_NOISE_CCP1305_HPP
