/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace libp2p {
  /**
   * Interface with destructor for `std::unique_ptr`.
   */
  class CancelDtor {
   public:
    virtual ~CancelDtor() = default;
  };

  /**
   * RAII object to cancel operation.
   */
  using Cancel = std::unique_ptr<CancelDtor>;

  template <typename F>
  class CancelDtorFn : public CancelDtor {
    F f;

   public:
    CancelDtorFn(F f) : f{std::move(f)} {}

    ~CancelDtorFn() override {
      f();
    }
  };

  Cancel cancelFn(auto fn) {
    return std::make_unique<CancelDtorFn<decltype(fn)>>(std::move(fn));
  }
}  // namespace libp2p
