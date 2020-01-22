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
  namespace hmac {
    class HmacProvider;
  }
  namespace rsa {
    class RsaProvider;
  }
  namespace ecdsa {
    class EcdsaProvider;
  }
  namespace secp256k1 {
    class Secp256k1Provider;
  }

  class CryptoProviderImpl : public CryptoProvider {
   public:
    enum class Error {
      UNKNOWN_CIPHER_TYPE = 1,
      UNKNOWN_HASH_TYPE,
    };

    ~CryptoProviderImpl() override = default;

    explicit CryptoProviderImpl(
        std::shared_ptr<random::CSPRNG> random_provider,
        std::shared_ptr<ed25519::Ed25519Provider> ed25519_provider,
        std::shared_ptr<rsa::RsaProvider> rsa_provider,
        std::shared_ptr<ecdsa::EcdsaProvider> ecdsa_provider,
        std::shared_ptr<secp256k1::Secp256k1Provider> secp256k1_provider,
        std::shared_ptr<hmac::HmacProvider> hmac_provider);

    outcome::result<KeyPair> generateKeys(
        Key::Type key_type, common::RSAKeyType rsa_bitness) const override;

    outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const override;

    outcome::result<Buffer> sign(gsl::span<const uint8_t> message,
                                 const PrivateKey &private_key) const override;

    outcome::result<bool> verify(gsl::span<const uint8_t> message,
                                 gsl::span<const uint8_t> signature,
                                 const PublicKey &public_key) const override;

    outcome::result<EphemeralKeyPair> generateEphemeralKeyPair(
        common::CurveType curve) const override;

    outcome::result<std::pair<StretchedKey, StretchedKey>> stretchKey(
        common::CipherType cipher_type, common::HashType hash_type,
        const Buffer &secret) const override;

   private:
    void initialize();
    static std::function<outcome::result<Buffer>(Buffer)>
    prepareSharedSecretGenerator(int curve_nid, Buffer own_private_key);

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
    outcome::result<Buffer> signEd25519(gsl::span<const uint8_t> message,
                                        const PrivateKey &private_key) const;
    outcome::result<bool> verifyEd25519(gsl::span<const uint8_t> message,
                                        gsl::span<const uint8_t> signature,
                                        const PublicKey &public_key) const;

    // Secp256k1
    outcome::result<KeyPair> generateSecp256k1() const;
    outcome::result<PublicKey> deriveSecp256k1(const PrivateKey &key) const;
    outcome::result<Buffer> signSecp256k1(gsl::span<const uint8_t> message,
                                          const PrivateKey &private_key) const;
    outcome::result<bool> verifySecp256k1(gsl::span<const uint8_t> message,
                                          gsl::span<const uint8_t> signature,
                                          const PublicKey &public_key) const;

    // ECDSA
    outcome::result<KeyPair> generateEcdsa() const;
    outcome::result<PublicKey> deriveEcdsa(const PrivateKey &key) const;
    outcome::result<Buffer> signEcdsa(gsl::span<const uint8_t> message,
                                      const PrivateKey &private_key) const;
    outcome::result<bool> verifyEcdsa(gsl::span<const uint8_t> message,
                                      gsl::span<const uint8_t> signature,
                                      const PublicKey &public_key) const;

    std::shared_ptr<random::CSPRNG> random_provider_;
    std::shared_ptr<ed25519::Ed25519Provider> ed25519_provider_;
    std::shared_ptr<rsa::RsaProvider> rsa_provider_;
    std::shared_ptr<ecdsa::EcdsaProvider> ecdsa_provider_;
    std::shared_ptr<secp256k1::Secp256k1Provider> secp256k1_provider_;
    std::shared_ptr<hmac::HmacProvider> hmac_provider_;
  };
}  // namespace libp2p::crypto

OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, CryptoProviderImpl::Error)

#endif  // LIBP2P_CRYPTO_PROVIDER_CRYPTO_PROVIDER_IMPL_HPP
