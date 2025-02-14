/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <utility>

#define NO_INLINE_DTOR(T, name) \
  ::libp2p::NoInlineDtor<T, struct NoInlineDtor_name_##name> name

namespace libp2p {
  // https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-noinline-function-attribute
  template <typename T, typename>
  struct NoInlineDtor : T {
    NoInlineDtor(const T &t) : T(t) {}
    NoInlineDtor(T &&t) : T(std::move(t)) {}
    __attribute__((noinline)) ~NoInlineDtor() {
      asm("");
    }
    using T::T;
    using T::operator=;
  };
}  // namespace libp2p
