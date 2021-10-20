/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>

template <typename T = size_t>
constexpr T spanSize(const gsl::span<const uint8_t> &span) {
  return gsl::narrow<T>(span.size());
}
