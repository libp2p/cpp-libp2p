/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <cstdint>
#include <qtils/cxx20/lexicographical_compare_three_way.hpp>
#include <span>
#include <vector>

namespace libp2p::common {
  /// Hash160 as a sequence of 20 bytes
  using Hash160 = std::array<uint8_t, 20u>;
  /// Hash256 as a sequence of 32 bytes
  using Hash256 = std::array<uint8_t, 32u>;
  /// Hash512 as a sequence of 64 bytes
  using Hash512 = std::array<uint8_t, 64u>;
}  // namespace libp2p::common

namespace libp2p {

  /// @brief convenience alias for arrays of bytes
  using Bytes = std::vector<uint8_t>;

  /// @brief convenience alias for immutable span of bytes
  using BytesIn = std::span<const uint8_t>;

  /// @brief convenience alias for mutable span of bytes
  using BytesOut = std::span<uint8_t>;

  template <class T>
  concept SpanOfBytes = std::is_same_v<std::decay_t<T>, BytesIn>
                     or std::is_same_v<std::decay_t<T>, BytesIn>;

  inline bool operator==(const SpanOfBytes auto &lhs,
                         const SpanOfBytes auto &rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
  }

  inline auto operator<=>(const SpanOfBytes auto &lhs,
                          const SpanOfBytes auto &rhs) {
    return qtils::cxx20::lexicographical_compare_three_way(
        lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
  }

}  // namespace libp2p
