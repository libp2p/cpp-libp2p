/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_RSA_TYPES_HPP
#define LIBP2P_CRYPTO_RSA_TYPES_HPP

#include <vector>

#include <libp2p/crypto/common.hpp>

namespace libp2p::crypto::rsa {
  /**
   * @brief Common types
   */
  using PrivateKey = std::vector<uint8_t>; /**< RSA private key */
  using PublicKey = std::vector<uint8_t>;  /**< RSA public key */
  using Signature = std::vector<uint8_t>;  /**< RSA signature of a message */
  using libp2p::crypto::common::RSAKeyType;

  struct KeyPair {
    PrivateKey private_key; /**< RSA private key */
    PublicKey public_key;   /**< RSA public key */

    /**
     * @brief Comparing keypairs
     * @param other - second keypair to compare
     * @return true, if keypairs are equal
     */
    bool operator==(const KeyPair &other) {
      return other.private_key == private_key && other.public_key == public_key;
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
};  // namespace libp2p::crypto::rsa

#endif  // LIBP2P_CRYPTO_RSA_TYPES_HPP
