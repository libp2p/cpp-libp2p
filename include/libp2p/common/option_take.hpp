/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

// TODO(turuslan): qtils, https://github.com/qdrvm/kagome/issues/1813
namespace qtils {
  /**
   * `std::move` doesn't reset `std::optional`.
   * Typical bug:
   *   std::optional<Fn> fn_once;
   *   if (fn_once) {
   *     // expected optional to be reset, but it wasn't
   *     auto fn = std::move(fn_once);
   *     fn();
   *   }
   *
   * `std::exchange` or `std::swap` don't compile because `std::optional` can't
   * be assigned.
   *
   * https://doc.rust-lang.org/std/option/enum.Option.html#method.take
   */
  template <typename T>
  auto optionTake(std::optional<T> &optional) {
    auto result = std::move(optional);
    optional.reset();
    return result;
  }
}  // namespace qtils
