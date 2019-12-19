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
        std::shared_ptr<hmac::HmacProvider> hmac_provider);

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

    outcome::result<std::pair<StretchedKey, StretchedKey>> stretchKey(
        common::CipherType cipher_type, common::HashType hash_type,
        const Buffer &secret) const override;

   private:
    void initialize();

    //    outcome::result<KeyPair> generateRsa(common::RSAKeyType key_type)
    //    const;
    auto generateEd25519() const -> outcome::result<KeyPair>;
    auto generateSecp256k1() const -> outcome::result<KeyPair>;
    auto generateEcdsa() const -> outcome::result<KeyPair>;

    auto deriveEd25519(const PrivateKey &key) const
        -> outcome::result<PublicKey>;
    auto signEd25519(gsl::span<uint8_t> message,
                     const PrivateKey &private_key) const
        -> outcome::result<Buffer>;
    auto verifyEd25519(gsl::span<uint8_t> message, gsl::span<uint8_t> signature,
                       const PublicKey &public_key) const
        -> outcome::result<bool>;

    auto generateEcdsa256WithCurve(Key::Type key_type, int curve_nid) const
        -> outcome::result<KeyPair>;
    auto prepareSharedSecretGenerator(int curve_nid,
                                      Buffer own_private_key) const
        -> std::function<outcome::result<Buffer>(Buffer)>;

    std::shared_ptr<random::CSPRNG> random_provider_;
    std::shared_ptr<ed25519::Ed25519Provider> ed25519_provider_;
    std::shared_ptr<hmac::HmacProvider> hmac_provider_;
  };
}  // namespace libp2p::crypto

OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, CryptoProviderImpl::Error)

#endif  // LIBP2P_CRYPTO_PROVIDER_CRYPTO_PROVIDER_IMPL_HPP
