/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

namespace testutil {
  template <typename T>
  struct TestParam {
    std::vector<uint8_t> encoded_value{};
    bool should_fail{false};
    T value{};
  };

  template <typename T>
  TestParam<T> make_param(std::vector<uint8_t> &&buffer,
                          bool should_fail,
                          T &&value) {
    return {buffer, should_fail, std::forward<T>(value)};
  };
}  // namespace testutil
