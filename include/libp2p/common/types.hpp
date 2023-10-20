/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_P2P_COMMON_TYPES_HPP
#define LIBP2P_P2P_COMMON_TYPES_HPP

#include <array>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

namespace libp2p::common {

  /// @brief convenience alias for arrays of bytes
  using ByteArray = std::vector<uint8_t>;

  template <typename Collection, typename Item>
  void append(Collection &c, Item &&g) {
    c.insert(c.end(), g.begin(), g.end());
  }

  template <typename Collection>
  void append(Collection &c, char g) {
    c.push_back(g);
  }

  /// Hash160 as a sequence of 20 bytes
  using Hash160 = std::array<uint8_t, 20u>;
  /// Hash256 as a sequence of 32 bytes
  using Hash256 = std::array<uint8_t, 32u>;
  /// Hash512 as a sequence of 64 bytes
  using Hash512 = std::array<uint8_t, 64u>;
}  // namespace libp2p::common

namespace libp2p {

  /// @brief convenience alias for immutable span of bytes
  using ConstSpanOfBytes = std::span<const uint8_t>;

  /// @brief convenience alias for mutable span of bytes
  using MutSpanOfBytes = std::span<uint8_t>;

  template <class T>
  concept SpanOfBytes = std::is_same_v<std::decay_t<T>, ConstSpanOfBytes>
      or std::is_same_v<std::decay_t<T>, ConstSpanOfBytes>;

  template <class T>
  concept RangeOfBytes =
      std::ranges::sized_range<T> and std::ranges::contiguous_range<T>
      and std::is_same_v<std::ranges::range_value_t<std::decay_t<T>>, uint8_t>;

  inline bool operator==(const SpanOfBytes auto &lhs,
                         const SpanOfBytes auto &rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }

  inline auto operator<=>(const SpanOfBytes auto &lhs,
                          const SpanOfBytes auto &rhs) {
    return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(),
                                                  rhs.begin(), rhs.end());
  }

}  // namespace libp2p

#endif  // LIBP2P_P2P_COMMON_TYPES_HPP
