/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
#define LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP

#include <algorithm>
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
     * @brief generators random byte
     * @return random byte
     */
    virtual uint8_t randomByte() = 0;

    /**
     * @brief generators random bytes
     * @param len number of bytes
     * @return buffer containing random bytes
     */
    virtual std::vector<uint8_t> randomBytes(size_t len) = 0;

    /**
     * @brief replaces container's elements by random bytes
     */
    template <typename Container>
    void fillRandomly(Container &container) {
      static_assert(std::is_integral_v<typename Container::value_type>);
      static_assert(sizeof(typename Container::value_type) == 1);
      std::transform(container.begin(), container.end(), container.begin(),
                     [this](auto) { return randomByte(); });
    }

    /**
     * @brief fills container by random bytes
     */
    template <typename Container>
    void fillRandomly(Container &container, size_t count) {
      static_assert(std::is_integral_v<typename Container::value_type>);
      static_assert(sizeof(typename Container::value_type) == 1);
      while (count--) {
        container.push_back(randomByte());
      }
    }
  };

  /**
   * @brief CSPRNG is a cryptographically secure pseudo random number generator.
   */
  class CSPRNG : public RandomGenerator {};
}  // namespace libp2p::crypto::random

#endif  // LIBP2P_CRYPTO_RANDOM_RANDOM_GENERATOR_HPP
