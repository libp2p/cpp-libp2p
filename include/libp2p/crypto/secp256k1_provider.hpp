/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_SECP256K1_PROVIDER_HPP
#define LIBP2P_CRYPTO_SECP256K1_PROVIDER_HPP

#include <gsl/span>
#include <libp2p/crypto/secp256k1_types.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::secp256k1 {

  /**
   * @class Secp256k1 provider interface
   */
  class Secp256k1Provider {
   public:
    /**
     * @brief Generate private and public keys
     * @return Secp256k1 key pair or error code
     */
    virtual outcome::result<KeyPair> generate() const = 0;

    /**
     * @brief Generate public key from private key
     * @param key - private key for deriving public key
     * @return Derived public key or error code
     */
    virtual outcome::result<PublicKey> derive(const PrivateKey &key) const = 0;

    /**
     * @brief Create signature for a message
     * @param message - data to signing
     * @param key - private key for signing
     * @return Secp256k1 signature or error code
     */
    virtual outcome::result<Signature> sign(gsl::span<const uint8_t> message,
                                            const PrivateKey &key) const = 0;

    /**
     * @brief Verify signature for a message
     * @param message - signed data
     * @param signature - target for verifying
     * @param key - public key for signature verifying
     * @return Result of the verification or error code
     */
    virtual outcome::result<bool> verify(gsl::span<const uint8_t> message,
                                         const Signature &signature,
                                         const PublicKey &key) const = 0;

    virtual ~Secp256k1Provider() = default;
  };
};  // namespace libp2p::crypto::secp256k1

#endif  // LIBP2P_CRYPTO_SECP256K1_PROVIDER_HPP
