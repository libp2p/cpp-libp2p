/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/random_generator/boost_generator.hpp"

namespace libp2p::crypto::random {

  std::vector<uint8_t> BoostRandomGenerator::randomBytes(size_t len) {
    std::vector<uint8_t> buffer(len, 0);
    for (size_t i = 0; i < len; ++i) {
      unsigned char byte = distribution_(generator_);
      buffer[i] = byte;  // NOLINT
    }
    return buffer;
  }
}  // namespace libp2p::crypto::random
