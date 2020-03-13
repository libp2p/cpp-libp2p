/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * base32 (de)coder implementation as specified by RFC4648.
 *
 * Copyright (c) 2010 Adrien Kunysz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/

#include <gsl/span>
#include <libp2p/multi/multibase_codec/codecs/base32.hpp>
#include <libp2p/multi/multibase_codec/codecs/base_error.hpp>

namespace libp2p::multi::detail {
  const std::string kUpperBase32Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
  const std::string kLowerBase32Alphabet = "abcdefghijklmnopqrstuvwxyz234567";

  enum Base32Mode {
    LOWER,
    UPPER,
  };

  int get_byte(int block) {
    return block * 5 / 8;
  }

  int get_bit(int block) {
    return 8 - 5 - block * 5 % 8;
  }

  char encode_char(unsigned char c, Base32Mode mode) {
    if (mode == Base32Mode::UPPER) {
      return kUpperBase32Alphabet[c & 0x1F];  // 0001 1111
    }
    return kLowerBase32Alphabet[c & 0x1F];
  }

  unsigned char shift_right(uint8_t byte, int8_t offset) {
    if (offset > 0)
      return byte >> offset;

    return byte << -offset;
  }

  unsigned char shift_left(uint8_t byte, int8_t offset) {
    return shift_right(byte, -offset);
  }

  int encode_sequence(gsl::span<const uint8_t> plain, gsl::span<char> coded,
                      Base32Mode mode) {
    for (int block = 0; block < 8; block++) {
      int byte = get_byte(block);
      int bit = get_bit(block);

      if (byte >= plain.size()) {
        return block;
      }

      unsigned char c = shift_right(plain[byte], bit);

      if (bit < 0 && byte < plain.size() - 1) {
        c |= shift_right(plain[byte + 1], 8 + bit);
      }
      coded[block] = encode_char(c, mode);
    }
    return 8;
  }

  std::string encodeBase32(const common::ByteArray &bytes, Base32Mode mode) {
    std::string result;
    if (bytes.size() % 5 == 0) {
      result = std::string(bytes.size() / 5 * 8, ' ');
    } else {
      result = std::string((bytes.size() / 5 + 1) * 8, ' ');
    }

    for (size_t i = 0, j = 0; i < bytes.size(); i += 5, j += 8) {
      int n = encode_sequence(
          gsl::make_span(&bytes[i], std::min<size_t>(bytes.size() - i, 5)),
          gsl::make_span(&result[j], 8), mode);
      if (n < 8) {
        result.erase(result.end() - (8 - n), result.end());
      }
    }

    return result;
  }

  std::string encodeBase32Upper(const common::ByteArray &bytes) {
    return encodeBase32(bytes, Base32Mode::UPPER);
  }

  std::string encodeBase32Lower(const common::ByteArray &bytes) {
    return encodeBase32(bytes, Base32Mode::LOWER);
  }

  int decode_char(unsigned char c, Base32Mode mode) {
    char decoded_ch = -1;

    if (mode == Base32Mode::UPPER) {
      if (c >= 'A' && c <= 'Z')
        decoded_ch = c - 'A';  // NOLINT
    } else {
      if (c >= 'a' && c <= 'z')
        decoded_ch = c - 'a';  // NOLINT
    }
    if (c >= '2' && c <= '7')
      decoded_ch = c - '2' + 26;  // NOLINT

    return decoded_ch;
  }

  outcome::result<int> decode_sequence(gsl::span<const char> coded,
                                       gsl::span<uint8_t> plain,
                                       Base32Mode mode) {
    plain[0] = 0;
    for (int block = 0; block < 8; block++) {
      int bit = get_bit(block);
      int byte = get_byte(block);

      if (block >= coded.size()) {
        return byte;
      }
      int c = decode_char(coded[block], mode);
      if (c < 0) {
        return BaseError::INVALID_BASE32_INPUT;
      }

      plain[byte] |= shift_left(c, bit);
      if (bit < 0) {
        plain[byte + 1] = shift_left(c, 8 + bit);
      }
    }
    return 5;
  }

  outcome::result<common::ByteArray> decodeBase32(std::string_view string,
                                                  Base32Mode mode) {
    common::ByteArray result;
    if (string.size() % 8 == 0) {
      result = common::ByteArray(string.size() / 8 * 5);
    } else {
      result = common::ByteArray((string.size() / 8 + 1) * 5);
    }

    for (size_t i = 0, j = 0; i < string.size(); i += 8, j += 5) {
      OUTCOME_TRY(n,
                  decode_sequence(
                      gsl::make_span(&string[i],
                                     std::min<size_t>(string.size() - i, 8)),
                      gsl::make_span(&result[j], 5), mode));
      if (n < 5) {
        result.erase(result.end() - (5 - n), result.end());
      }
    }
    return result;
  }

  outcome::result<common::ByteArray> decodeBase32Upper(
      std::string_view string) {
    return decodeBase32(string, Base32Mode::UPPER);
  }

  outcome::result<common::ByteArray> decodeBase32Lower(
      std::string_view string) {
    return decodeBase32(string, Base32Mode::LOWER);
  }

}  // namespace libp2p::multi::detail
