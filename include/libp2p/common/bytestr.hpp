/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_BYTESTR_HPP
#define LIBP2P_COMMON_BYTESTR_HPP

#include <cstdint>
#include <gsl/span>

namespace libp2p {
  inline gsl::span<const uint8_t> bytestr(const gsl::span<const char> &s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
  }
}  // namespace libp2p

#endif  // LIBP2P_COMMON_BYTESTR_HPP
