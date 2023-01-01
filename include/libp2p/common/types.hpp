/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_P2P_COMMON_TYPES_HPP
#define LIBP2P_P2P_COMMON_TYPES_HPP

#include <array>
#include <cstdint>
#include <vector>

namespace libp2p::common {
  /**
   * Sequence of bytes
   */
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

#endif  // LIBP2P_P2P_COMMON_TYPES_HPP
