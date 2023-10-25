/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/chachapoly/chachapoly_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {
  class NoiseCCP1305Impl : public AEADCipher {
   public:
    NoiseCCP1305Impl(Key32 key);

    outcome::result<Bytes> encrypt(BytesIn precompiled_out,
                                   uint64_t nonce,
                                   BytesIn plaintext,
                                   BytesIn aad) override;

    outcome::result<Bytes> decrypt(BytesIn precompiled_out,
                                   uint64_t nonce,
                                   BytesIn ciphertext,
                                   BytesIn aad) override;

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
