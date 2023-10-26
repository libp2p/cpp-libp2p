/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace libp2p {
  // TODO(turuslan): https://github.com/libp2p/cpp-libp2p/issues/203
  template <typename T>
  void ambigousSize(std::span<T> &s, size_t n) {
    if (n > s.size()) {
      throw std::logic_error{"libp2p::ambigousSize"};
    }
    s = s.first(n);
  }
}  // namespace libp2p
