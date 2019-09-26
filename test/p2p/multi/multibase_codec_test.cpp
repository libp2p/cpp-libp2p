/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"

using namespace libp2p::multi;
using namespace kagome::common;

class MultibaseCodecTest : public ::testing::Test {
 public:
  std::unique_ptr<MultibaseCodec> multibase =
      std::make_unique<MultibaseCodecImpl>();

  /**
   * Decode the string
   * @param encoded - string with encoding prefix to be decoded into bytes; MUST
   * be valid encoded string
   * @return resulting Multibase
   */
  Buffer decodeCorrect(std::string_view encoded) {
    auto multibase_opt = multibase->decode(encoded);
    EXPECT_TRUE(multibase_opt.has_value())
        << "failed to decode string: " + std::string{encoded};
    return multibase_opt.value();
  }
};

TEST_F(MultibaseCodecTest, EncodeEmptyBytes) {
  auto encoded_str =
      multibase->encode(Buffer{}, MultibaseCodec::Encoding::BASE16_LOWER);
  ASSERT_TRUE(encoded_str.empty());
}

/**
 * @given string with encoding prefix, which does not stand for any of the
 * implemented encodings
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(MultibaseCodecTest, DecodeIncorrectPrefix) {
  auto multibase_err = multibase->decode("J00AA");
  ASSERT_FALSE(multibase_err.has_value());
}

/**
 * @given string of length 1
 * @when trying to decode that string
 * @then Multibase object creation fails
 */
TEST_F(MultibaseCodecTest, DecodeFewCharacters) {
  auto multibase_err = multibase->decode("A");
  ASSERT_FALSE(multibase_err.has_value());
}

class Base16EncodingUpper : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::BASE16_UPPER;

  std::string_view encoded_correct{"F00010204081020FF"};
  Buffer decoded_correct{0, 1, 2, 4, 8, 16, 32, 255};

  std::string_view encoded_incorrect_prefix{"fAA"};
  std::string_view encoded_incorrect_body{"F10A"};
};

/**
 * @given uppercase hex-encoded string
 * @when trying to decode that string
 * @then decoding succeeds
 */
TEST_F(Base16EncodingUpper, SuccessDecoding) {
  auto decoded_bytes = decodeCorrect(encoded_correct);
  ASSERT_EQ(decoded_bytes, decoded_correct);
}

/**
 * @given bytes
 * @when trying to encode those bytes
 * @then encoding succeeds
 */
TEST_F(Base16EncodingUpper, SuccessEncoding) {
  auto encoded_str = multibase->encode(decoded_correct, encoding);
  ASSERT_EQ(encoded_str, encoded_correct);
}

/**
 * @given uppercase hex-encoded string with lowercase hex prefix
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base16EncodingUpper, IncorrectPrefix) {
  auto error = multibase->decode(encoded_incorrect_prefix);
  ASSERT_FALSE(error.has_value());
}

/**
 * @given non-hex-encoded string with uppercase prefix
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base16EncodingUpper, IncorrectBody) {
  auto error = multibase->decode(encoded_incorrect_body);
  ASSERT_FALSE(error.has_value());
}

class Base16EncodingLower : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::BASE16_LOWER;

  std::string_view encoded_correct{"f00010204081020ff"};
  Buffer decoded_correct{0, 1, 2, 4, 8, 16, 32, 255};

  std::string_view encoded_incorrect_prefix{"Faa"};
  std::string_view encoded_incorrect_body{"f10a"};
};

/**
 * @given lowercase hex-encoded string
 * @when trying to decode that string
 * @then decoding succeeds
 */
TEST_F(Base16EncodingLower, SuccessDecoding) {
  auto decoded_bytes = decodeCorrect(encoded_correct);
  ASSERT_EQ(decoded_bytes, decoded_correct);
}

/**
 * @given bytes
 * @when trying to encode those bytes
 * @then encoding succeeds
 */
TEST_F(Base16EncodingLower, SuccessEncoding) {
  auto encoded_str = multibase->encode(decoded_correct, encoding);
  ASSERT_EQ(encoded_str, encoded_correct);
}

/**
 * @given lowercase hex-encoded string with uppercase hex prefix
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base16EncodingLower, IncorrectPrefix) {
  auto error = multibase->decode(encoded_incorrect_prefix);
  ASSERT_FALSE(error.has_value());
}

/**
 * @given non-hex-encoded string with lowercase prefix
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base16EncodingLower, IncorrectBody) {
  auto error = multibase->decode(encoded_incorrect_body);
  ASSERT_FALSE(error.has_value());
}

class Base58Encoding : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::BASE58;

  const std::vector<std::pair<Buffer, std::string_view>> decode_encode_table{
      {Buffer{"61"_unhex}, "Z2g"},
      {Buffer{"626262"_unhex}, "Za3gV"},
      {Buffer{"636363"_unhex}, "ZaPEr"},
      {Buffer{"73696d706c792061206c6f6e6720737472696e67"_unhex},
       "Z2cFupjhnEsSn59qHXstmK2ffpLv2"},
      {Buffer{"00eb15231dfceb60925886b67d065299925915aeb172c06647"_unhex},
       "Z1NS17iag9jJgTHD1VXjvLCEnZuQ3rJDE9L"},
      {Buffer{"516b6fcd0f"_unhex}, "ZABnLTmg"},
      {Buffer{"bf4f89001e670274dd"_unhex}, "Z3SEo3LWLoPntC"},
      {Buffer{"572e4794"_unhex}, "Z3EFU7m"},
      {Buffer{"ecac89cad93923c02321"_unhex}, "ZEJDM8drfXA6uyA"},
      {Buffer{"10c8511e"_unhex}, "ZRt5zm"},
      {Buffer{"00000000000000000000"_unhex}, "Z1111111111"},
      {Buffer{"000111d38e5fc9071ffcd20b4a763cc9ae4f252bb4e"
              "48fd66a835e252ada93ff480d6dd43dc62a641155a5"_unhex},
       "Z123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"},
      {Buffer{
           "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"
           "2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f4041"
           "42434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162"
           "636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f80818283"
           "8485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4"
           "a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5"
           "c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6"
           "e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"_unhex},
       "Z1cWB5HCBdLjAuqGGReWE3R3CguuwSjw6RHn39s2yuDRTS5NsBgNiFpWgAnEx6VQi8"
       "c"
       "sexkgYw3mdYrMHr8x9i7aEwP8kZ7vccXWqKDvGv3u1GxFKPuAkn8JCPPGDMf3vMMnb"
       "z"
       "m6Nh9zh1gcNsMvH3ZNLmP5fSG6DGbbi2tuwMWPthr4boWwCxf7ewSgNQeacyozhKDD"
       "Q"
       "Q1qL5fQFUW52QKUZDZ5fw3KXNQJMcNTcaB723LchjeKun7MuGW5qyCBZYzA1KjofN1"
       "g"
       "YBV3NqyhQJ3Ns746GNuf9N2pQPmHz4xpnSrrfCvy6TVVz5d4PdrjeshsWQwpZsZGzv"
       "b"
       "dAdN8MKV5QsBDY"}};

  static constexpr std::string_view incorrect_encoded{"Z1c0I5H"};
};

/**
 * @given table with base58-encoded strings with their bytes representations
 * @when encoding bytes @and decoding strings
 * @then encoding/decoding succeed @and relevant bytes and strings are
 * equivalent
 */
TEST_F(Base58Encoding, SuccessEncodingDecoding) {
  for (const auto &[decoded, encoded] : decode_encode_table) {
    auto encoded_str = multibase->encode(decoded, encoding);
    ASSERT_EQ(encoded_str, encoded);

    auto decoded_bytes = decodeCorrect(encoded);
    ASSERT_EQ(decoded_bytes, decoded);
  }
}

/**
 * @given string containing symbols, forbidden in base58
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base58Encoding, IncorrectBody) {
  auto error = multibase->decode(incorrect_encoded);
  ASSERT_FALSE(error.has_value());
}

/**
 * Check that whitespace characters are skipped as intended
 * @given base58-encoded string with several whitespaces @and valid base58
 * symbols in the middle @and more whitespaces
 * @when trying to decode that string
 * @then decoding succeeds
 */
TEST_F(Base58Encoding, SkipsWhitespacesSuccess) {
  constexpr auto base64_with_whitespaces = "Z \t\n\v\f\r 2g \r\f\v\n\t ";
  auto decoded_bytes = decodeCorrect(base64_with_whitespaces);

  ASSERT_EQ(decoded_bytes, Buffer{"61"_unhex});
}

/**
 * Check that unexpected symbol in the end prevents success decoding
 * @given base58-encoded string with several whitespaces @and valid base58
 * symbols in the middle @and more whitespaces @and base58 character
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base58Encoding, SkipsWhitespacesFailure) {
  constexpr auto base64_with_whitespaces = "Z \t\n\v\f\r skip \r\f\v\n\t a";
  auto error = multibase->decode(base64_with_whitespaces);

  ASSERT_FALSE(error.has_value());
}

class Base64Encoding : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::BASE64;

  const std::vector<std::pair<Buffer, std::string_view>> decode_encode_table{
      {Buffer{"66"_unhex}, "mZg=="},
      {Buffer{"666f"_unhex}, "mZm8="},
      {Buffer{"666f6f"_unhex}, "mZm9v"},
      {Buffer{"666f6f62"_unhex}, "mZm9vYg=="},
      {Buffer{"666f6f6261"_unhex}, "mZm9vYmE="},
      {Buffer{"666f6f626172"_unhex}, "mZm9vYmFy"},
      {Buffer{
           "4d616e2069732064697374696e677569736865642c206e6f74206f6e6c7920627"
           "92068697320726561736f6e2c2062757420627920746869732073696e67756c61"
           "722070617373696f6e2066726f6d206f7468657220616e696d616c732c2077686"
           "963682069732061206c757374206f6620746865206d696e642c20746861742062"
           "792061207065727365766572616e6365206f662064656c6967687420696e20746"
           "86520636f6e74696e75656420616e6420696e6465666174696761626c65206765"
           "6e65726174696f6e206f66206b6e6f776c656467652c206578636565647320746"
           "8652073686f727420766568656d656e6365206f6620616e79206361726e616c20"
           "706c6561737572652e"_unhex},
       "mTWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBi"
       "eS"
       "B0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyB"
       "hI"
       "Gx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodC"
       "Bp"
       "biB0aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd"
       "2x"
       "lZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVh"
       "c3"
       "VyZS4="},
  };

  static constexpr std::string_view incorrect_encoded{"m1c0=5H"};
};

/**
 * @given table with base64-encoded strings with their bytes representations
 * @when encoding bytes @and decoding strings
 * @then encoding/decoding succeed @and relevant bytes and strings are
 * equivalent
 */
TEST_F(Base64Encoding, SuccessEncodingDecoding) {
  for (const auto &[decoded, encoded] : decode_encode_table) {
    auto encoded_str = multibase->encode(decoded, encoding);
    ASSERT_EQ(encoded_str, encoded);

    auto decoded_bytes = decodeCorrect(encoded);
    ASSERT_EQ(decoded_bytes, decoded);
  }
}

/**
 * @given string containing symbols, forbidden in base64
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base64Encoding, IncorrectBody) {
  auto error = multibase->decode(incorrect_encoded);
  ASSERT_FALSE(error.has_value());
}
