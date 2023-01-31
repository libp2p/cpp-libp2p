/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_AMBIGOUS_SIZE_HPP
#define LIBP2P_COMMON_AMBIGOUS_SIZE_HPP

#include <cstdint>
#include <libp2p/common/span_size.hpp>

namespace libp2p {
  // TODO(turuslan): https://github.com/libp2p/cpp-libp2p/issues/203
  template <typename T>
  void ambigousSize(gsl::span<T> &s, size_t n) {
    if (n > spanSize(s)) {
      throw std::logic_error{"libp2p::ambigousSize"};
    }
    s = s.first(n);
  }
}  // namespace libp2p

#endif  // LIBP2P_COMMON_AMBIGOUS_SIZE_HPP
