/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace libp2p::crypto {
  /**
   * Supported types of all keys
   */
  enum class KeyType {
    UNSPECIFIED = 100,
    RSA = 0,
    Ed25519 = 1,
    Secp256k1 = 2,
    ECDSA = 3
  };
}  // namespace libp2p::crypto
