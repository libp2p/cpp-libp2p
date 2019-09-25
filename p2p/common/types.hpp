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

  /**
   * Hash256 as a sequence of 32 bytes
   */
  using Hash256 = std::array<uint8_t, 32u>;
}  // namespace libp2p::common

#endif  // LIBP2P_P2P_COMMON_TYPES_HPP
