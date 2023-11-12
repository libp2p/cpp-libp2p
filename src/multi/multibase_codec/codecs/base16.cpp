/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/multibase_codec/codecs/base16.hpp>

#include <algorithm>
#include <cctype>

#include <libp2p/multi/multibase_codec/codecs/base_error.hpp>
#include <qtils/hex.hpp>
#include <qtils/unhex.hpp>

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
  std::string encodeBase16Upper(BytesIn bytes) {
    return fmt::format("{:X}", bytes);
  }

  std::string encodeBase16Lower(BytesIn bytes) {
    return fmt::format("{:x}", bytes);
  }

  outcome::result<Bytes> decodeBase16Upper(std::string_view string) {
    // we need this check, because Boost can unhex any kind of base16 with one
    // func, but the base must be specified correctly
    if (!encodingCaseIsUpper(string)) {
      return Q_ERROR(BaseError::NON_UPPERCASE_INPUT);
    }
    return qtils::unhex(string);
  }

  outcome::result<Bytes> decodeBase16Lower(std::string_view string) {
    // we need this check, because Boost can unhex any kind of base16 with one
    // func, but the base must be specified correctly
    if (encodingCaseIsUpper(string)) {
      return Q_ERROR(BaseError::NON_LOWERCASE_INPUT);
    }
    return qtils::unhex(string);
  }

}  // namespace libp2p::multi::detail
