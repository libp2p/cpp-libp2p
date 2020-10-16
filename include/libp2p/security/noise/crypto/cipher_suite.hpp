/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_CIPHER_SUITE_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_CIPHER_SUITE_HPP

#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {

  class CipherSuiteImpl : public CipherSuite {
   public:
    CipherSuiteImpl(std::shared_ptr<DiffieHellman> dh,
                    std::shared_ptr<NamedHasher> hash,
                    std::shared_ptr<NamedAEADCipher> cipher);

    outcome::result<DHKey> generate() override;

    outcome::result<ByteArray> dh(const ByteArray &private_key,
                                  const ByteArray &public_key) override;

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
#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_CIPHER_SUITE_HPP
