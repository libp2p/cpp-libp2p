/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/multibase_codec/codecs/base16.hpp>

#include <algorithm>
#include <cctype>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/multi/multibase_codec/codecs/base_error.hpp>

namespace {
  /**
   * Check, if hex string is in uppercase
   * @param string to be checked
   * @return true, if provided string is uppercase hex-encoded, false otherwise
   */
  bool encodingCaseIsUpper(std::string_view string) {
    return std::all_of(string.begin(), string.end(), [](const char &c) {
      // NOLINTNEXTLINE(readability-implicit-bool-conversion)
      return !std::isalpha(c) || static_cast<bool>(std::isupper(c));
    });
  }
}  // namespace

namespace libp2p::multi::detail {

  using common::ByteArray;
  using common::hex_lower;
  using common::hex_upper;
  using common::unhex;

  std::string encodeBase16Upper(const ByteArray &bytes) {
    return hex_upper(bytes);
  }

  std::string encodeBase16Lower(const ByteArray &bytes) {
    return hex_lower(bytes);
  }

  outcome::result<ByteArray> decodeBase16Upper(std::string_view string) {
    // we need this check, because Boost can unhex any kind of base16 with one
    // func, but the base must be specified correctly
    if (!encodingCaseIsUpper(string)) {
      return BaseError::NON_UPPERCASE_INPUT;
    }
    OUTCOME_TRY(bytes, unhex(string));
    return ByteArray{std::move(bytes)};
  }

  outcome::result<ByteArray> decodeBase16Lower(std::string_view string) {
    // we need this check, because Boost can unhex any kind of base16 with one
    // func, but the base must be specified correctly
    if (encodingCaseIsUpper(string)) {
      return BaseError::NON_LOWERCASE_INPUT;
    }
    OUTCOME_TRY(bytes, unhex(string));
    return ByteArray{std::move(bytes)};
  }

}  // namespace libp2p::multi::detail
