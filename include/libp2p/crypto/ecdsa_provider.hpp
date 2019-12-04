/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_ECDSA_PROVIDER_HPP
#define LIBP2P_CRYPTO_ECDSA_PROVIDER_HPP

#include <gsl/span>
#include <libp2p/crypto/ecdsa_types.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::ecdsa {
  class EcdsaProvider {
   public:
    /**
     * @brief Generate private and public keys
     * @return ECDSA key pair or error code
     */
    virtual outcome::result<KeyPair> GenerateKeyPair() const = 0;

    /**
     * @brief Generate ECDSA public key from private key
     * @param key - ECDSA private key
     * @return Generated public key or error code
     */
    virtual outcome::result<PublicKey> DerivePublicKey(const PrivateKey &key) const = 0;

    /**
     * @brief Create signature for a message
     * @param message - data to signing
     * @param privateKey - key for signing
     * @return ECDSA signature or error code
     */
    virtual outcome::result<Signature> Sign(gsl::span<uint8_t> message,
                                            const PrivateKey &key) const = 0;

    /**
     * @brief Verify signature for a message
     * @param message - signed data
     * @param signature - target for verifying
     * @param publicKey - key for signature verifying
     * @return Result of the verification or error code
     */
    virtual outcome::result<bool> Verify(gsl::span<uint8_t> message,
                                         const Signature &signature,
                                         const PublicKey &publicKey) const = 0;

    virtual ~EcdsaProvider() = default;
  };
};  // namespace libp2p::crypto::ecdsa

#endif  // LIBP2P_CRYPTO_ECDSA_PROVIDER_HPP
