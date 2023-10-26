/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {

  class CipherSuiteImpl : public CipherSuite {
   public:
    CipherSuiteImpl(std::shared_ptr<DiffieHellman> dh,
                    std::shared_ptr<NamedHasher> hash,
                    std::shared_ptr<NamedAEADCipher> cipher);

    outcome::result<DHKey> generate() override;

    outcome::result<Bytes> dh(const Bytes &private_key,
                              const Bytes &public_key) override;

    int dhSize() const override;

    std::string dhName() const override;

    std::shared_ptr<crypto::Hasher> hash() override;

    std::string hashName() const override;

    std::shared_ptr<AEADCipher> cipher(Key32 key) override;

    std::string cipherName() const override;

    std::string name() override;

   private:
    std::shared_ptr<DiffieHellman> dh_;
    std::shared_ptr<NamedHasher> hash_;
    std::shared_ptr<NamedAEADCipher> cipher_;
  };

}  // namespace libp2p::security::noise
