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
    inline virtual ~CancelDtor() = 0;
  };
  CancelDtor::~CancelDtor() {}

  /**
   * RAII object to cancel operation.
   */
  using Cancel = std::unique_ptr<CancelDtor>;

  template <typename F>
  class CancelDtorFn final : public CancelDtor {
    F f;

   public:
    explicit CancelDtorFn(F f) : f{std::move(f)} {}

    ~CancelDtorFn() override {
      f();
    }

    // clang-tidy cppcoreguidelines-special-member-functions
    CancelDtorFn(const CancelDtorFn &) = delete;
    void operator=(const CancelDtorFn &) = delete;
    CancelDtorFn(CancelDtorFn &&) = delete;
    void operator=(CancelDtorFn &&) = delete;
  };

  Cancel cancelFn(auto fn) {
    return std::make_unique<CancelDtorFn<decltype(fn)>>(std::move(fn));
  }
}  // namespace libp2p
