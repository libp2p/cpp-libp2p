/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_RANDOM_BOOST_GENERATOR_HPP
#define LIBP2P_CRYPTO_RANDOM_BOOST_GENERATOR_HPP

#include <boost/nondet_random.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <libp2p/crypto/random_generator.hpp>

namespace libp2p::crypto::random {
  /**
   * @class BoostRandomGenerator provides implementation
   * of cryptographic-secure random bytes generator;
   * on systems which don't provide true random numbers source
   * it may not compile, so you will need to implement
   * your own random bytes generator
   */
  class BoostRandomGenerator : public CSPRNG {
   public:
    ~BoostRandomGenerator() override = default;

    /**
     * @brief generates a random byte
     * @return random byte
     */
    uint8_t randomByte() override;

    /**
     * @brief generators random bytes
     * @param len number of bytes
     * @return buffer containing random bytes
     */
    std::vector<uint8_t> randomBytes(size_t len) override;

   private:
    /// boost cryptographic-secure random generator
    boost::random_device generator_;
    /// uniform distribution tool
    boost::random::uniform_int_distribution<uint8_t> distribution_;
  };
}  // namespace libp2p::crypto::random

#endif  // LIBP2P_CRYPTO_RANDOM_BOOST_GENERATOR_HPP
