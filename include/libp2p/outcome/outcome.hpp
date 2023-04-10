/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_OUTCOME_HPP
#define LIBP2P_OUTCOME_HPP

#include <boost/outcome/result.hpp>
#include <boost/outcome/success_failure.hpp>
#include <boost/outcome/try.hpp>

// To define OUTCOME_TRY macro, we will need to create OUTCOME_TRY_1 and
// OUTCOME_TRY_2 depending on number of arguments
#define OUTCOME_TRY_1(...) BOOST_OUTCOME_TRY(__VA_ARGS__)
#define OUTCOME_TRY_2(...) BOOST_OUTCOME_TRY(auto __VA_ARGS__)

// trick from https://stackoverflow.com/a/11763277 to overload OUTCOME_TRY
#define GET_MACRO(_1, _2, NAME, ...) NAME
#define OUTCOME_TRY(...) \
  GET_MACRO(__VA_ARGS__, OUTCOME_TRY_2, OUTCOME_TRY_1)(__VA_ARGS__)

#include <libp2p/outcome/outcome-register.hpp>

/**
 * __cpp_sized_deallocation macro interferes with protobuf generated files
 * and makes them uncompilable by means of clang
 * since it is not necessary outside of outcome internals
 * it can be safely undefined
 */
#ifdef __cpp_sized_deallocation
#undef __cpp_sized_deallocation
#endif

namespace libp2p::outcome {
  using namespace BOOST_OUTCOME_V2_NAMESPACE;  // NOLINT

  template <class R, class S = std::error_code,
            class NoValuePolicy = policy::default_policy<R, S, void>>  //
  using result = basic_result<R, S, NoValuePolicy>;

}  // namespace libp2p::outcome

// @see /docs/result.md

#endif  // LIBP2P_OUTCOME_HPP
