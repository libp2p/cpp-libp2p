/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_RANDOM_GENERATOR_MOCK_HPP
#define LIBP2P_RANDOM_GENERATOR_MOCK_HPP

#include <libp2p/crypto/random_generator.hpp>

#include <gmock/gmock.h>

namespace libp2p::crypto::random {
  struct RandomGeneratorMock : public RandomGenerator {
    ~RandomGeneratorMock() override = default;

    MOCK_METHOD0(randomByte, uint8_t());
    MOCK_METHOD1(randomBytes, std::vector<uint8_t>(size_t));
  };

  struct CSPRNGMock : public CSPRNG {
    ~CSPRNGMock() override = default;

	  MOCK_METHOD0(randomByte, uint8_t());
    MOCK_METHOD1(randomBytes, std::vector<uint8_t>(size_t));
  };
}  // namespace libp2p::crypto::random

#endif  // LIBP2P_RANDOM_GENERATOR_MOCK_HPP
