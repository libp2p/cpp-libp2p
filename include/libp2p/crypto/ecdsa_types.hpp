/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_ECDSA_TYPES_HPP
#define LIBP2P_CRYPTO_ECDSA_TYPES_HPP

#include <array>
#include <vector>

namespace libp2p::crypto::ecdsa {
  /**
   * @brief Common types
   */
  using PrivateKey = std::array<uint8_t, 121>;
  using PublicKey = std::array<uint8_t, 91>;
  using Signature = std::vector<uint8_t>;
  using PrehashedMessage = std::array<uint8_t, 32>;

  /**
   * @struct Key pair
   */
  struct KeyPair {
    PrivateKey private_key; /**< ECDSA private key */
    PublicKey public_key;   /**< ECDSA public key */

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
};  // namespace libp2p::crypto::ecdsa

#endif  // LIBP2P_CRYPTO_ECDSA_TYPES_HPP
