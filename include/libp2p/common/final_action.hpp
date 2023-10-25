/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <utility>

namespace libp2p::common {

  template <typename F>
  struct FinalAction {
    FinalAction() = delete;
    FinalAction(FinalAction &&func) = delete;
    FinalAction(const FinalAction &func) = delete;
    FinalAction &operator=(FinalAction &&func) = delete;
    FinalAction &operator=(const FinalAction &func) = delete;

    FinalAction(F &&func) : func(std::forward<F>(func)) {}

    ~FinalAction() {
      func();
    }

   public:
    // To prevent an object being created on the heap
    void *operator new(std::size_t) = delete;            // standard new
    void *operator new(std::size_t, void *) = delete;    // placement new
    void *operator new[](std::size_t) = delete;          // array new
    void *operator new[](std::size_t, void *) = delete;  // placement array new

   private:
    F func;
  };

  template <typename F>
  FinalAction(F &&) -> FinalAction<F>;

  template <typename F>
  struct MovableFinalAction {
    MovableFinalAction() = delete;
    MovableFinalAction(MovableFinalAction &&func) = default;
    MovableFinalAction(const MovableFinalAction &func) = delete;
    MovableFinalAction &operator=(MovableFinalAction &&func) = default;
    MovableFinalAction &operator=(const MovableFinalAction &func) = delete;

    MovableFinalAction(F &&func) : func(std::forward<F>(func)) {}

    ~MovableFinalAction() {
      if (func.has_value()) {
        func->operator()();
      }
    }

   private:
    std::optional<F> func;
  };

  template <typename F>
  MovableFinalAction(F &&) -> MovableFinalAction<F>;

}  // namespace libp2p::common
