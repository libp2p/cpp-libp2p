/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_BYTESTR_HPP
#define LIBP2P_COMMON_BYTESTR_HPP

#include <cstdint>

#include <libp2p/common/types.hpp>

namespace libp2p {

  inline BytesIn bytestr(const std::span<const char> &s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
  }

}  // namespace libp2p

#endif  // LIBP2P_COMMON_BYTESTR_HPP
