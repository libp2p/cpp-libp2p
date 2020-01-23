/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_PROVIDER_HPP
#define LIBP2P_CRYPTO_PROVIDER_HPP

#include <vector>

#include <boost/filesystem.hpp>
#include <gsl/span>
#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/key.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto {
  /**
   * @class CryptoProvider provides interface for key generation, singing,
   * signature verification functions for private/public key cryptography
   */
  class CryptoProvider {
   public:
    using Buffer = std::vector<uint8_t>;

    virtual ~CryptoProvider() = default;

    /**
     * @brief generates new key pair of specified type
     * @param key_type key type
     * @param rsa_bitness specifies the length of RSA key
     * @return new generated key pair of public and private key or error
     */
    virtual outcome::result<KeyPair> generateKeys(
        Key::Type key_type,
        common::RSAKeyType rsa_bitness = common::RSAKeyType::RSA2048) const = 0;

    /**
     * @brief derives public key from private key
     * @param private_key private key
     * @return derived public key or error
     */
    virtual outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const = 0;

    /**
     * @brief signs a given message using passed private key
     * @param message bytes to be signed
     * @param private_key a part of keypair used for signature generation
     * @return signature bytes
     */
    virtual outcome::result<Buffer> sign(
        gsl::span<const uint8_t> message,
        const PrivateKey &private_key) const = 0;

    /**
     * @brief verifies validness of the signature for a given message and public
     * key
     * @param message that was signed
     * @param signature to be validated
     * @param public_key to validate against
     * @return true - if the signature matches the message and the public key
     */
    virtual outcome::result<bool> verify(gsl::span<const uint8_t> message,
                                         gsl::span<const uint8_t> signature,
                                         const PublicKey &public_key) const = 0;
    /**
     * Generate an ephemeral public key and return a function that will
     * compute the shared secret key
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
    virtual outcome::result<std::pair<StretchedKey, StretchedKey>> stretchKey(
        common::CipherType cipher_type, common::HashType hash_type,
        const Buffer &secret) const = 0;
  };

}  // namespace libp2p::crypto

#endif  // LIBP2P_CRYPTO_PROVIDER_HPP
