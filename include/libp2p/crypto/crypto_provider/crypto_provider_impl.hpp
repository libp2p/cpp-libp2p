/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_PROVIDER_CRYPTO_PROVIDER_IMPL_HPP
#define LIBP2P_CRYPTO_PROVIDER_CRYPTO_PROVIDER_IMPL_HPP

#include <libp2p/crypto/crypto_provider.hpp>

namespace libp2p::crypto {
  namespace random {
    class CSPRNG;
  }

  class CryptoProviderImpl : public CryptoProvider {
   public:
    ~CryptoProviderImpl() override = default;

    explicit CryptoProviderImpl(random::CSPRNG &random_provider);

    outcome::result<KeyPair> generateKeys(Key::Type key_type) const override;

    outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const override;

    outcome::result<Buffer> sign(gsl::span<uint8_t> message,
                                 const PrivateKey &private_key) const override;

    outcome::result<bool> verify(gsl::span<uint8_t> message,
                                 gsl::span<uint8_t> signature,
                                 const PublicKey &public_key) const override;

    outcome::result<EphemeralKeyPair> generateEphemeralKeyPair(
        common::CurveType curve) const override;

    std::vector<StretchedKey> stretchKey(common::CipherType cipher_type,
                                         common::HashType hash_type,
                                         const Buffer &secret) const override;

   private:
    void initialize();

    //    outcome::result<KeyPair> generateRsa(common::RSAKeyType key_type)
    //    const;
    outcome::result<KeyPair> generateEd25519() const;
    outcome::result<KeyPair> generateSecp256k1() const;
    outcome::result<KeyPair> generateEcdsa() const;

    outcome::result<KeyPair> generateEcdsa256WithCurve(Key::Type key_type,
                                                       int curve_nid) const;

    random::CSPRNG &random_provider_;  ///< random bytes generator
  };
}  // namespace libp2p::crypto

#endif  // LIBP2P_CRYPTO_PROVIDER_CRYPTO_PROVIDER_IMPL_HPP
