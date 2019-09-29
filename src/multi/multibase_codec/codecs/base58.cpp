/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/multibase_codec/codecs/base58.hpp>

#include <array>
#include <cstring>

#include <boost/optional.hpp>
#include <libp2p/multi/multibase_codec/codecs/base_error.hpp>

namespace {
  // All alphanumeric characters except for "0", "I", "O", and "l"
  constexpr std::string_view pszBase58 =
      "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

  // clang-format off
  constexpr std::array<int8_t, 256> mapBase58 = {
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
      -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
      22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
      -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
      47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  };
  // clang-format on

  /**
   * Tests if the given character is a whitespace character. The whitespace
   * characters are: space, form-feed ('\f'), newline ('\n'), carriage return
   * ('\r'), horizontal tab ('\t'), and vertical tab ('\v')
   * @param c - char to be tested
   * @return true, if char is space, false otherwise
   */
  constexpr bool isSpace(char c) noexcept {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t'
        || c == '\v';
  }
}  // namespace

namespace libp2p::multi::detail {

  /**
   * Actual implementation of the encoding
   * @param pbegin - pointer to the beginning of bytes collection
   * @param pend - pointer to the end of bytes collection
   * @return encoded string
   */
  std::string encodeImpl(const unsigned char *pbegin,
                         const unsigned char *pend) {
    // Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;
    while (pbegin != pend && *pbegin == 0) {
      std::advance(pbegin, 1);
      zeroes++;
    }
    // Allocate enough space in big-endian base58 representation.
    // log(256) / log(58), rounded up.
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
    int size = (pend - pbegin) * 138 / 100 + 1;
    std::vector<unsigned char> b58(size);
    // Process the bytes.
    while (pbegin != pend) {
      int carry = *pbegin;
      int i = 0;
      // Apply "b58 = b58 * 256 + ch".
      for (auto it = b58.rbegin();
           (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
        carry += 256 * (*it);
        *it = carry % 58;
        carry /= 58;
      }

      length = i;
      std::advance(pbegin, 1);
    }
    // Skip leading zeroes in base58 result.
    auto it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0) {
      it++;
    }
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end()) {
      str += pszBase58[*it];
      std::advance(it, 1);
    }
    return str;
  }

  /**
   * Actual implementation of the decoding
   * @param psz - pointer to the string to be decoded
   * @return decoded bytes, if the process went successfully, none otherwise
   */
  boost::optional<std::vector<unsigned char>> decodeImpl(const char *psz) {
    // Skip leading spaces.
    while ((*psz != '0') && isSpace(*psz)) {
      std::advance(psz, 1);
    }
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') {
      zeroes++;
      std::advance(psz, 1);
    }
    // Allocate enough space in big-endian base256 representation.
    // log(58) / log(256), rounded up.
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
    int size = strlen(psz) * 733 / 1000 + 1;
    std::vector<unsigned char> b256(size);
    // Process the characters.
    while (*psz && !isSpace(*psz)) {  // NOLINT
      // Decode base58 character
      int carry = mapBase58.at(static_cast<uint8_t>(*psz));
      if (carry == -1) {  // Invalid b58 character
        return {};
      }
      int i = 0;
      for (auto it = b256.rbegin();
           (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
        carry += 58 * (*it);
        *it = carry % 256;
        carry /= 256;
      }
      if (carry != 0) {
        return {};
      }
      length = i;
      std::advance(psz, 1);
    }
    // Skip trailing spaces.
    while (isSpace(*psz)) {
      std::advance(psz, 1);
    }
    if (*psz != 0) {
      return {};
    }
    // Skip leading zeroes in b256.
    auto it = b256.begin() + (size - length);
    while (it != b256.end() && *it == 0) {
      it++;
    }
    // Copy result into output vector.
    std::vector<unsigned char> vch(zeroes + (b256.end() - it));
    vch.assign(zeroes, 0x00);
    while (it != b256.end()) {
      vch.push_back(*(it++));
    }
    return vch;
  }

  std::string encodeBase58(const common::ByteArray &bytes) {
    return encodeImpl(bytes.begin().base(), bytes.end().base());
  }

  outcome::result<common::ByteArray> decodeBase58(std::string_view string) {
    auto decoded_bytes = decodeImpl(string.data());
    if (decoded_bytes) {
      return common::ByteArray{*decoded_bytes};
    }
    return BaseError::INVALID_BASE58_INPUT;
  }

}  // namespace libp2p::multi::detail
