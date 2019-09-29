/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/random_generator/boost_generator.hpp>

#include <cmath>

#include <gtest/gtest.h>
#include <gsl/span>

using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::random::RandomGenerator;

/**
 * @given 2 instances of boost random numbers generators
 * @when each generator's randomBytes method is called to generate to buffers of
 * random nubmers
 * @then obtained byte sequences are not equal
 * This test checks that random bytes generator dosn't start with the same
 * sequence each time it has been created
 */
TEST(BoostGeneratorTest, StartSequencesAreNotSame) {
  BoostRandomGenerator generator1, generator2;
  constexpr size_t BUFFER_SIZE = 32;

  auto bytes1 = generator1.randomBytes(BUFFER_SIZE);
  auto bytes2 = generator2.randomBytes(BUFFER_SIZE);

  ASSERT_NE(bytes1, bytes2);
}

namespace {
  /**
   * Taken from
   * https://github.com/hyperledger/iroha-ed25519/blob/master/test/randombytes/random.cpp#L6
   * @brief calculates entropy of byte sequence
   * @param sequence  source byte sequence
   * @return entropy value
   */
  double entropy(gsl::span<uint8_t> sequence) {
    std::vector<uint8_t> freqs(256, 0);

    // calculate frequencies
    for (unsigned char i : sequence) {
      ++freqs[i];
    }

    double e = 0;
    for (const auto c : freqs) {
      double freq = (double)c / sequence.size();

      if (freq == 0) {
        continue;
      }

      e += freq * std::log2(freq);
    }

    return -e;
  }

  /**
   * @brief returns max possible entropy for a buffer of given size
   * @param volume is volume of alphabet
   * @return max value of entropy for given source alphabet volume
   */
  double max_entropy(size_t volume) {
    return log2(volume);
  }
}  // namespace

/**
 * @brief checks quality of random bytes generator
 * @param generator implementation of generator
 * @return true if quality is good enough false otherwise
 */
bool checkRandomGenerator(RandomGenerator &generator) {
  constexpr size_t BUFFER_SIZE = 256;

  auto buffer = generator.randomBytes(BUFFER_SIZE);

  auto max = max_entropy(256);  // 8
  auto ent = entropy(buffer);

  return ent >= (max - 2);
}

/**
 * @given BoostRandomGenerator instance, max entropy value for given source
 * @when get random bytes and calculate entropy
 * @then calculated entropy is not less than (max entropy - 2)
 */
TEST(BoostGeneratorTest, EnoughEntropy) {
  BoostRandomGenerator generator;
  bool res = checkRandomGenerator(static_cast<RandomGenerator &>(generator));
  ASSERT_TRUE(res) << "bad randomness source in BoostRandomGenerator";
}
