/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/outcome.hpp>

/**
 * Boost 1.87.0 (1.86.0?) removed overloads with `error_code` and only kept
 * throwing overload.
 */
namespace boost_outcome {
  template <typename F>
  outcome::result<std::invoke_result_t<F>> tryCatch(const F &f) {
    try {
      return f();
    } catch (boost::system::system_error &e) {
      return e.code();
    }
  }

  outcome::result<std::string> to_string(const auto &x) {
    return tryCatch([&] { return x.to_string(); });
  }
}  // namespace boost_outcome
