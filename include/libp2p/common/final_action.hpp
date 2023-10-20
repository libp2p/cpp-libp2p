/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>

namespace libp2p::common {

  struct FinalAction {
    FinalAction(std::function<void()> &&func)
        : func(std::forward<std::function<void()>>(func)) {}

    ~FinalAction() {
      func();
    }

   public:
    // To prevent an object being created on the heap
    void *operator new(size_t) = delete;            // standard new
    void *operator new(size_t, void *) = delete;    // placement new
    void *operator new[](size_t) = delete;          // array new
    void *operator new[](size_t, void *) = delete;  // placement array new

   private:
    std::function<void()> func;
  };

}  // namespace libp2p::common
