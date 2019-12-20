/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_SECP256K1_TYPES_HPP
#define LIBP2P_CRYPTO_SECP256K1_TYPES_HPP

#include <vector>
#include <array>

namespace libp2p::crypto::secp256k1 {

  static const size_t kPrivateKeyLength = 32;
  static const size_t kPublicKeyLength = 33;
  /**
   * @brief Common types
   */
  using PrivateKey = std::array<uint8_t, kPrivateKeyLength>;
  using PublicKey = std::array<uint8_t, kPublicKeyLength>;
  using Signature = std::vector<uint8_t>;

  /**
   * @struct Key pair
   */
  struct KeyPair {
    PrivateKey private_key; /**< Secp256k1 private key */
    PublicKey public_key;   /**< Secp256k1 public key */

    /**
     * @brief Comparing keypairs
     * @param other - second keypair to compare
     * @return true, if keypairs are equal
     */
    bool operator==(const KeyPair &other) {
      return private_key == other.private_key && public_key == other.public_key;
    }

    /**
     * @brief Comparing keypairs
     * @param other - second keypair to compare
     * @return true, if keypairs aren't equal
     */
    bool operator!=(const KeyPair &other) {
      return !operator==(other);
    }
  };
};  // namespace libp2p::crypto::secp256k1

#endif  // LIBP2P_CRYPTO_SECP256K1_TYPES_HPP
