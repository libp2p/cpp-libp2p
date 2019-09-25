/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP

#include <cstdint>
#include <vector>

namespace libp2p::crypto::random {

  /**
   * @brief RandomGenerator is a basic interface to (not necessarily
   * cryptographically secure) pseudo random number generator.
   */
  class RandomGenerator {
   public:
    virtual ~RandomGenerator() = default;
    /**
     * @brief generators random bytes
     * @param len number of bytes
     * @return buffer containing random bytes
     */
    virtual std::vector<uint8_t> randomBytes(size_t len) = 0;
  };

  /**
   * @brief CSPRNG is a cryptographically secure pseudo random number generator.
   */
  class CSPRNG : public RandomGenerator {
   public:
    ~CSPRNG() override = default;

    std::vector<uint8_t> randomBytes(size_t len) override = 0;
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
