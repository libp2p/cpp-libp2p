/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

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

  int encode_sequence(const uint8_t *plain, int len, char *coded,
                      Base32Mode mode) {
    for (int block = 0; block < 8; block++) {
      int byte = get_byte(block);
      int bit = get_bit(block);

      if (byte >= len) {
        return block;
      }

      unsigned char c = shift_right(plain[byte], bit);  // NOLINT

      if (bit < 0 && byte < len - 1) {
        c |= shift_right(plain[byte + 1], 8 + bit);  // NOLINT
      }
      coded[block] = encode_char(c, mode);  // NOLINT
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
      int n = encode_sequence(&bytes[i], std::min<size_t>(bytes.size() - i, 5),
                              &result[j], mode);
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

  outcome::result<int> decode_sequence(const char *coded, int len,
                                       uint8_t *plain, Base32Mode mode) {
    plain[0] = 0;  // NOLINT
    for (int block = 0; block < 8; block++) {
      int bit = get_bit(block);
      int byte = get_byte(block);

      if (block >= len) {
        return byte;
      }
      int c = decode_char(coded[block], mode);  // NOLINT
      if (c < 0) {
        return BaseError::INVALID_BASE32_INPUT;
      }

      plain[byte] |= shift_left(c, bit);  // NOLINT
      if (bit < 0) {
        plain[byte + 1] = shift_left(c, 8 + bit);  // NOLINT
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
      OUTCOME_TRY(
          n,
          decode_sequence(&string[i], std::min<size_t>(string.size() - i, 8),
                          &result[j], mode));
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
