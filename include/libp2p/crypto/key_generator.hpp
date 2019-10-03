/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_KEY_GENERATOR_HPP
#define LIBP2P_CRYPTO_KEY_GENERATOR_HPP

#include <boost/filesystem.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/key.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <vector>

namespace libp2p::crypto {
  /**
   * @class KeyGenerator provides interface for key generation functions
   */
  class KeyGenerator {
   public:
    using Buffer = std::vector<uint8_t>;

    virtual ~KeyGenerator() = default;

    /**
     * @brief generates new key pair of specified type
     * @param key_type key type
     * @return new generated key pair of public and private key or error
     */
    virtual outcome::result<KeyPair> generateKeys(Key::Type key_type) const = 0;

    /**
     * @brief derives public key from private key
     * @param private_key private key
     * @return derived public key or error
     */
    virtual outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const = 0;
    /**
     * Generate an ephemeral public key and return a function that will compute
     * the shared secret key
     * @param curve to be used in this ECDH
     * @return ephemeral key pair
     */
    virtual outcome::result<EphemeralKeyPair> generateEphemeralKeyPair(
        common::CurveType curve) const = 0;

    /**
     * Generate a set of keys for each party by stretching the shared key
     * @param cipher_type to be used
     * @param hash_type to be used
     * @param secret to be used
     * @return objects of type StretchedKey
     */
    virtual std::vector<StretchedKey> stretchKey(
        common::CipherType cipher_type, common::HashType hash_type,
        const Buffer &secret) const = 0;
  };

}  // namespace libp2p::crypto

#endif  // LIBP2P_CRYPTO_KEY_GENERATOR_HPP
