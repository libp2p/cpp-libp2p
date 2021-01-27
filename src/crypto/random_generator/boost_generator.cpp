/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/random_generator/boost_generator.hpp>

namespace libp2p::crypto::random {

  uint8_t BoostRandomGenerator::randomByte() {
    return distribution_(generator_);  // NOLINT
  }

  std::vector<uint8_t> BoostRandomGenerator::randomBytes(size_t len) {
    std::vector<uint8_t> buffer(len, 0);
    for (size_t i = 0; i < len; ++i) {
      unsigned char byte = BoostRandomGenerator::randomByte();
      buffer[i] = byte;  // NOLINT
    }
    return buffer;
  }
}  // namespace libp2p::crypto::random
