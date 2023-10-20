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

    outcome::result<ByteArray> encrypt(ConstSpanOfBytes precompiled_out,
                                       uint64_t nonce,
                                       ConstSpanOfBytes plaintext,
                                       ConstSpanOfBytes aad) override;

    outcome::result<ByteArray> decrypt(ConstSpanOfBytes precompiled_out,
                                       uint64_t nonce,
                                       ConstSpanOfBytes ciphertext,
                                       ConstSpanOfBytes aad) override;

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
