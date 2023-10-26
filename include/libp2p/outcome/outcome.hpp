/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/outcome/result.hpp>
#include <boost/outcome/success_failure.hpp>
#include <boost/outcome/try.hpp>

#include <fmt/format.h>

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

  template <class R,
            class S = std::error_code,
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
    const auto &message = ec.message();
    return std::copy(message.begin(), message.end(), ctx.out());
  }
};

template <>
struct fmt::formatter<boost::system::error_code> {
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

  // Formats the boost::system::error_code
  template <typename FormatContext>
  auto format(const boost::system::error_code &ec, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    const auto &message = ec.message();
    return std::copy(std::begin(message), std::end(message), ctx.out());
  }
};

// Remove after it will be added to libp2p (will be happened compilation error)
template <typename T>
struct fmt::formatter<libp2p::outcome::success_type<T>> {
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

  // Formats the success<non_void_type>
  template <typename OutputIt>
  typename std::enable_if_t<not std::is_void_v<T>, OutputIt> format_impl(
      OutputIt out, const libp2p::outcome::success_type<T> &success) const {
    return fmt::format_to(out, "{}", success.value());
  }

  // Formats the success<void>
  template <typename OutputIt>
  typename std::enable_if_t<std::is_void_v<T>, OutputIt> format_impl(
      OutputIt out, const libp2p::outcome::success_type<void> &) const {
    static constexpr string_view message("<success>");
    return std::copy(std::begin(message), std::end(message), out);
  }

  // Formats the success<T>
  template <typename FormatContext>
  auto format(const libp2p::outcome::success_type<T> &success,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return format_impl(ctx.out(), success);
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
        return fmt::format_to(ctx.out(), "{}", res.value());
      } else {
        static constexpr string_view message("<success>");
        return std::copy(std::begin(message), std::end(message), ctx.out());
      }
    } else {
      return fmt::format_to(ctx.out(), "{}", res.error());
    }
  }
};
