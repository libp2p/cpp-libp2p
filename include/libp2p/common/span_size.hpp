/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_SPAN_SIZE_HPP
#define LIBP2P_COMMON_SPAN_SIZE_HPP

#include <gsl/span>

namespace libp2p {
  template <typename T>
  size_t spanSize(const gsl::span<T> &s) {
    return s.size();
  }
}  // namespace libp2p

#endif  // LIBP2P_COMMON_SPAN_SIZE_HPP
