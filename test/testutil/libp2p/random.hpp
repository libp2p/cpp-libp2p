/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <random>
#include <vector>

namespace testutil {

  /**
   * Returns a deterministic RNG for tests. Same seed every run for reproducible
   * tests; avoids flaky behavior from unseeded rand().
   * NOTE: Shared RNG; test execution order affects the sequence (determinism
   * is per run, not per test).
   */
  inline std::mt19937& getTestRng() {
    static std::mt19937 rng(0x42);  // arbitrary fixed seed for reproducibility
    return rng;
  }

  /**
   * Fills a buffer with deterministic random bytes. Use in tests instead of
   * rand() for reproducibility.
   */
  inline std::vector<uint8_t> randomBytes(size_t n) {
    std::vector<uint8_t> buf(n);
    auto& rng = getTestRng();
    std::uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < n; ++i) {
      buf[i] = static_cast<uint8_t>(dist(rng));
    }
    return buf;
  }

}  // namespace testutil
