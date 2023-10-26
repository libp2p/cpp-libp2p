/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
