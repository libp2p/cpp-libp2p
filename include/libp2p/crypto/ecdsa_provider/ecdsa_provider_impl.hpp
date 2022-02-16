/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_ECDSA_PROVIDER_IMPL_HPP
#define LIBP2P_CRYPTO_ECDSA_PROVIDER_IMPL_HPP

#include <memory>

#include <openssl/ec.h>
#include <libp2p/crypto/ecdsa_provider.hpp>

namespace libp2p::crypto::ecdsa {

  class EcdsaProviderImpl : public EcdsaProvider {
   public:
    outcome::result<KeyPair> generate() const override;

    outcome::result<PublicKey> derive(const PrivateKey &key) const override;

    outcome::result<Signature> sign(gsl::span<const uint8_t> message,
                                    const PrivateKey &key) const override;

    outcome::result<Signature> signPrehashed(
        const PrehashedMessage &message, const PrivateKey &key) const override;

    outcome::result<bool> verify(gsl::span<const uint8_t> message,
                                 const Signature &signature,
                                 const PublicKey &key) const override;

    outcome::result<bool> verifyPrehashed(
        const PrehashedMessage &message, const Signature &signature,
        const PublicKey &public_key) const override;

   private:
    /**
     * @brief Convert EC_KEY to bytes
     * @tparam KeyType - private or public key
     * @param ec_key - source to convert
     * @param converter - OpenSSL function
     * @return Converted key or error code
     */
    template <typename KeyType>
    outcome::result<KeyType> convertEcKeyToBytes(
        const std::shared_ptr<EC_KEY> &ec_key,
        int (*converter)(EC_KEY *, uint8_t **)) const;

    /**
     * @brief Convert bytes to EC_KEY
     * @tparam KeyType - private or public key
     * @param key - source to convert
     * @param converter - OpenSSL function
     * @return Converted key or error code
     */
    template <typename KeyType>
    outcome::result<std::shared_ptr<EC_KEY>> convertBytesToEcKey(
        const KeyType &key,
        EC_KEY *(*converter)(EC_KEY **, const uint8_t **, long)) const;
  };
}  // namespace libp2p::crypto::ecdsa

#endif  // LIBP2P_CRYPTO_ECDSA_PROVIDER_IMPL_HPP
