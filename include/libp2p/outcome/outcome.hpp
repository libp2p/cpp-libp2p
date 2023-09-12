/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_OUTCOME_HPP
#define LIBP2P_OUTCOME_HPP

#include <boost/outcome/result.hpp>
#include <boost/outcome/success_failure.hpp>
#include <boost/outcome/try.hpp>

#include <soralog/common.hpp>

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

template <>
struct fmt::formatter<std::error_code> {
  // Parses format specifications. Must be empty
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the std::error_code
  template <typename FormatContext>
  auto format(const std::error_code &ec, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    return soralog::fmt::format_to(ctx.out(), "{}", ec.message());
  }
};

template <typename Result, typename Failure>
struct fmt::formatter<libp2p::outcome::result<Result, Failure>> {
  // Parses format specifications. Must be empty
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the outcome result
  template <typename FormatContext>
  auto format(const libp2p::outcome::result<Result, Failure> &res,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (res.has_value()) {
      if constexpr (not std::is_void_v<Result>) {
        return soralog::fmt::format_to(ctx.out(), "{}", res.value());
      } else {
        return soralog::fmt::format_to(ctx.out(), "<success>");
      }
    } else {
      return soralog::fmt::format_to(ctx.out(), "{}", res.error());
    }
  }
};

#endif  // LIBP2P_OUTCOME_HPP
