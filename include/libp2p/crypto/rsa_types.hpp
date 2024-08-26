/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/key_type.hpp>

namespace libp2p::crypto::rsa {
  /**
   * @brief Common types
   */
  using PrivateKey = std::vector<uint8_t>; /**< RSA private key */
  using PublicKey = std::vector<uint8_t>;  /**< RSA public key */
  using Signature = std::vector<uint8_t>;  /**< RSA signature of a message */
  using libp2p::crypto::common::RSAKeyType;

  struct KeyPair {
    static constexpr auto key_type = KeyType::RSA;
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
