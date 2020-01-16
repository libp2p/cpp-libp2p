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
  namespace ed25519 {
    class Ed25519Provider;
  }
  namespace rsa {
    class RsaProvider;
  }

  class CryptoProviderImpl : public CryptoProvider {
   public:
    ~CryptoProviderImpl() override = default;

    explicit CryptoProviderImpl(
        std::shared_ptr<random::CSPRNG> random_provider,
        std::shared_ptr<ed25519::Ed25519Provider> ed25519_provider,
        std::shared_ptr<rsa::RsaProvider> rsa_provider);

    outcome::result<KeyPair> generateKeys(
        Key::Type key_type, common::RSAKeyType rsa_bitness) const override;

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

    // RSA
    outcome::result<KeyPair> generateRsa(common::RSAKeyType rsa_bitness) const;
    outcome::result<PublicKey> deriveRsa(const PrivateKey &key) const;
    outcome::result<Buffer> signRsa(gsl::span<const uint8_t> message,
                                    const PrivateKey &private_key) const;
    outcome::result<bool> verifyRsa(gsl::span<const uint8_t> message,
                                    gsl::span<const uint8_t> signature,
                                    const PublicKey &public_key) const;

    // Ed25519
    outcome::result<KeyPair> generateEd25519() const;
    outcome::result<PublicKey> deriveEd25519(const PrivateKey &key) const;
    outcome::result<Buffer> signEd25519(gsl::span<uint8_t> message,
                                        const PrivateKey &private_key) const;
    outcome::result<bool> verifyEd25519(gsl::span<uint8_t> message,
                                        gsl::span<uint8_t> signature,
                                        const PublicKey &public_key) const;

    // Secp256k1
    outcome::result<KeyPair> generateSecp256k1() const;

    // EcDSA
    outcome::result<KeyPair> generateEcdsa() const;

    outcome::result<KeyPair> generateEcdsa256WithCurve(Key::Type key_type,
                                                       int curve_nid) const;

    std::shared_ptr<random::CSPRNG> random_provider_;
    std::shared_ptr<ed25519::Ed25519Provider> ed25519_provider_;
    std::shared_ptr<rsa::RsaProvider> rsa_provider_;
  };
}  // namespace libp2p::crypto

#endif  // LIBP2P_CRYPTO_PROVIDER_CRYPTO_PROVIDER_IMPL_HPP
