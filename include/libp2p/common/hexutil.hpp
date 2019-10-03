/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_HEXUTIL_HPP
#define LIBP2P_HEXUTIL_HPP

#include <string_view>
#include <vector>

#include <gsl/span>

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::common {

  /**
   * @brief error codes for exceptions that may occur during unhexing
   */
  enum class UnhexError { NOT_ENOUGH_INPUT = 1, NON_HEX_INPUT, UNKNOWN };

  /**
   * @brief Converts an integer to an uppercase hex representation
   */
  std::string int_to_hex(uint64_t n, size_t fixed_width = 2) noexcept;

  /**
   * @brief Converts bytes to uppercase hex representation
   * @param array bytes
   * @param len length of bytes
   * @return hexstring
   */
  std::string hex_upper(gsl::span<const uint8_t> bytes) noexcept;

  /**
   * @brief Converts bytes to hex representation
   * @param array bytes
   * @param len length of bytes
   * @return hexstring
   */
  std::string hex_lower(gsl::span<const uint8_t> bytes) noexcept;

  /**
   * @brief Converts hex representation to bytes
   * @param array individual chars
   * @param len length of chars
   * @return result containing array of bytes if input string is hex encoded and
   * has even length
   *
   * @note reads both uppercase and lowercase hexstrings
   *
   * @see
   * https://www.boost.org/doc/libs/1_51_0/libs/algorithm/doc/html/the_boost_algorithm_library/Misc/hex.html
   */
  outcome::result<std::vector<uint8_t>> unhex(std::string_view hex);

}  // namespace libp2p::common

OUTCOME_HPP_DECLARE_ERROR(libp2p::common, UnhexError);

#endif  // LIBP2P_HEXUTIL_HPP
