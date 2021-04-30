/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/outcome/policy/result_error_code_throw_as_system_error.hpp>

namespace libp2p::outcome::no_error {
  using boost::outcome_v2::policy::base;
  using boost::outcome_v2::policy::error_code_throw_as_system_error;

  void report(const void *ptr, size_t size, uint32_t status, const std::type_info &type);

  template <typename T>
  struct Policy : error_code_throw_as_system_error<T, std::error_code, void> {
    template <class Impl>
    static constexpr void wide_error_check(Impl &&self) {
      if (!base::_has_error(std::forward<Impl>(self))) {
        report(&self, sizeof(self), self._iostreams_state()._status, typeid(T));
      }
    }
  };
}  // namespace libp2p::outcome::no_error
