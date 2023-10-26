/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace libp2p {
  template <typename T>
  struct SharedFn {
    explicit SharedFn(T &&f) : f{std::make_shared<T>(std::move(f))} {};

    template <typename... A>
    auto operator()(A &&...a) const {
      return f->operator()(std::forward<A>(a)...);
    }

    std::shared_ptr<T> f;
  };
}  // namespace libp2p
