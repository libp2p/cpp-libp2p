/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/multibase_codec/multibase_codec_impl.hpp>

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/common/types.hpp>

using namespace libp2p::multi;
using namespace libp2p::common;

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
  ByteArray decodeCorrect(std::string_view encoded) {
    auto multibase_opt = multibase->decode(encoded);
    EXPECT_TRUE(multibase_opt.has_value())
        << "failed to decode string: " + std::string{encoded};
    return multibase_opt.value();
  }
};

TEST_F(MultibaseCodecTest, EncodeEmptyBytes) {
  auto encoded_str =
      multibase->encode(ByteArray{}, MultibaseCodec::Encoding::BASE16_LOWER);
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
  ByteArray decoded_correct{0, 1, 2, 4, 8, 16, 32, 255};

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
  ByteArray decoded_correct{0, 1, 2, 4, 8, 16, 32, 255};

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

class Base32EncodingLower : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::BASE32_LOWER;

  const std::vector<std::pair<ByteArray, std::string_view>> decode_encode_table{
      {"61"_unhex, "bme"},
      {"626262"_unhex, "bmjrge"},
      {"636363"_unhex, "bmnrwg"},
      {"73696d706c792061206c6f6e6720737472696e67"_unhex,
       "bonuw24dmpeqgcidmn5xgoidtorzgs3th"},
      {"00eb15231dfceb60925886b67d065299925915aeb172c06647"_unhex,
       "badvrkiy57tvwbesyq23h2bsstgjfsfnowfzmazsh"},
      {"516b6fcd0f"_unhex, "bkfvw7tip"},
      {"bf4f89001e670274dd"_unhex, "bx5hysaa6m4bhjxi"},
      {"572e4794"_unhex, "bk4xepfa"},
      {"ecac89cad93923c02321"_unhex, "b5switswzher4aizb"},
      {"10c8511e"_unhex, "bcdefchq"},
      {"00000000000000000000"_unhex, "baaaaaaaaaaaaaaaa"},
      {"000111d38e5fc9071ffcd20b4a763cc9ae4f252bb4e"
       "48fd66a835e252ada93ff480d6dd43dc62a641155a5"_unhex,
       "baaardu4ol7eqoh742ifuu5r4zgxe6jjlwtsi7vtkqnpckkw2sp7uqdln2q64mktecfk2"
       "k"},
      {"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"
       "2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f4041"
       "42434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162"
       "636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f80818283"
       "8485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4"
       "a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5"
       "c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6"
       "e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"_unhex,
       "baaaqeayeaudaocajbifqydiob4ibceqtcqkrmfyydenbwha5dypsaijcemsckjrhfausuk"
       "zmfuxc6mbrgiztinjwg44dsor3hq6t4p2aifbegrcfizduqskkjnge2tspkbiveu2ukvlfo"
       "wczljnvyxk6l5qgcytdmrswmz3infvgw3dnnzxxa4lson2hk5txpb4xu634pv7h7aebqkby"
       "jbmgq6eitculrsgy5d4qsgjjhfevs2lzrgm2tooj3hu7ucq2fi5euwtkpkfjvkv2zlnov6y"
       "ldmvtws23nn5yxg5lxpf5x274bqocypcmlrwhzde4vs6mzxhm7ugr2lj5jvow27mntww33t"
       "o55x7a4hrohzhf43t6r2pk5pwo33xp6dy7f47u6x3pp6hz7l57z7p674"}};

  static constexpr std::string_view incorrect_encoded{"BMe"};
};

/**
 * @given bytes
 * @when trying to encode those bytes
 * @then encoding succeeds
 */
TEST_F(Base32EncodingLower, SuccessEncoding) {
  for (const auto &[decoded, encoded] : decode_encode_table) {
    auto encoded_str = multibase->encode(decoded, encoding);
    ASSERT_EQ(encoded_str, encoded);

    auto decoded_bytes = decodeCorrect(encoded);
    ASSERT_EQ(decoded_bytes, decoded);
  }
}

/**
 * @given string containing symbols, forbidden in base32 lower case
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base32EncodingLower, IncorrectBody) {
  auto error = multibase->decode(incorrect_encoded);
  ASSERT_FALSE(error.has_value());
}

class Base32EncodingUpper : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::BASE32_UPPER;

  const std::vector<std::pair<ByteArray, std::string_view>> decode_encode_table{
      {"61"_unhex, "BME"},
      {"626262"_unhex, "BMJRGE"},
      {"636363"_unhex, "BMNRWG"},
      {"73696d706c792061206c6f6e6720737472696e67"_unhex,
       "BONUW24DMPEQGCIDMN5XGOIDTORZGS3TH"},
      {"00eb15231dfceb60925886b67d065299925915aeb172c06647"_unhex,
       "BADVRKIY57TVWBESYQ23H2BSSTGJFSFNOWFZMAZSH"},
      {"516b6fcd0f"_unhex, "BKFVW7TIP"},
      {"bf4f89001e670274dd"_unhex, "BX5HYSAA6M4BHJXI"},
      {"572e4794"_unhex, "BK4XEPFA"},
      {"ecac89cad93923c02321"_unhex, "B5SWITSWZHER4AIZB"},
      {"10c8511e"_unhex, "BCDEFCHQ"},
      {"00000000000000000000"_unhex, "BAAAAAAAAAAAAAAAA"},
      {"000111d38e5fc9071ffcd20b4a763cc9ae4f252bb4e"
       "48fd66a835e252ada93ff480d6dd43dc62a641155a5"_unhex,
       "BAAARDU4OL7EQOH742IFUU5R4ZGXE6JJLWTSI7VTKQNPCKKW2SP7UQDLN2Q64MKTECFK2"
       "K"},
      {"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"
       "2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f4041"
       "42434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162"
       "636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f80818283"
       "8485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4"
       "a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5"
       "c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6"
       "e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"_unhex,
       "BAAAQEAYEAUDAOCAJBIFQYDIOB4IBCEQTCQKRMFYYDENBWHA5DYPSAIJCEMSCKJRHFAUSUK"
       "ZMFUXC6MBRGIZTINJWG44DSOR3HQ6T4P2AIFBEGRCFIZDUQSKKJNGE2TSPKBIVEU2UKVLFO"
       "WCZLJNVYXK6L5QGCYTDMRSWMZ3INFVGW3DNNZXXA4LSON2HK5TXPB4XU634PV7H7AEBQKBY"
       "JBMGQ6EITCULRSGY5D4QSGJJHFEVS2LZRGM2TOOJ3HU7UCQ2FI5EUWTKPKFJVKV2ZLNOV6Y"
       "LDMVTWS23NN5YXG5LXPF5X274BQOCYPCMLRWHZDE4VS6MZXHM7UGR2LJ5JVOW27MNTWW33T"
       "O55X7A4HROHZHF43T6R2PK5PWO33XP6DY7F47U6X3PP6HZ7L57Z7P674"}};

  static constexpr std::string_view incorrect_encoded{"BMe"};
};

/**
 * @given bytes
 * @when trying to encode those bytes
 * @then encoding succeeds
 */
TEST_F(Base32EncodingUpper, SuccessEncoding) {
  for (const auto &[decoded, encoded] : decode_encode_table) {
    auto encoded_str = multibase->encode(decoded, encoding);
    ASSERT_EQ(encoded_str, encoded);

    auto decoded_bytes = decodeCorrect(encoded);
    ASSERT_EQ(decoded_bytes, decoded);
  }
}

/**
 * @given string containing symbols, forbidden in base32 upper case
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base32EncodingUpper, IncorrectBody) {
  auto error = multibase->decode(incorrect_encoded);
  ASSERT_FALSE(error.has_value());
}

class Base58Encoding : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::BASE58;

  const std::vector<std::pair<ByteArray, std::string_view>> decode_encode_table{
      {"61"_unhex, "z2g"},
      {"626262"_unhex, "za3gV"},
      {"636363"_unhex, "zaPEr"},
      {"73696d706c792061206c6f6e6720737472696e67"_unhex,
       "z2cFupjhnEsSn59qHXstmK2ffpLv2"},
      {"00eb15231dfceb60925886b67d065299925915aeb172c06647"_unhex,
       "z1NS17iag9jJgTHD1VXjvLCEnZuQ3rJDE9L"},
      {"516b6fcd0f"_unhex, "zABnLTmg"},
      {"bf4f89001e670274dd"_unhex, "z3SEo3LWLoPntC"},
      {"572e4794"_unhex, "z3EFU7m"},
      {"ecac89cad93923c02321"_unhex, "zEJDM8drfXA6uyA"},
      {"10c8511e"_unhex, "zRt5zm"},
      {"00000000000000000000"_unhex, "z1111111111"},
      {"000111d38e5fc9071ffcd20b4a763cc9ae4f252bb4e"
       "48fd66a835e252ada93ff480d6dd43dc62a641155a5"_unhex,
       "z123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"},
      {"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"
       "2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f4041"
       "42434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162"
       "636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f80818283"
       "8485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4"
       "a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5"
       "c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6"
       "e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"_unhex,
       "z1cWB5HCBdLjAuqGGReWE3R3CguuwSjw6RHn39s2yuDRTS5NsBgNiFpWgAnEx6VQi8"
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
  constexpr auto base64_with_whitespaces = "z \t\n\v\f\r 2g \r\f\v\n\t ";
  auto decoded_bytes = decodeCorrect(base64_with_whitespaces);

  ASSERT_EQ(decoded_bytes, "61"_unhex);
}

/**
 * Check that unexpected symbol in the end prevents success decoding
 * @given base58-encoded string with several whitespaces @and valid base58
 * symbols in the middle @and more whitespaces @and base58 character
 * @when trying to decode that string
 * @then decoding fails
 */
TEST_F(Base58Encoding, SkipsWhitespacesFailure) {
  constexpr auto base64_with_whitespaces = "z \t\n\v\f\r skip \r\f\v\n\t a";
  auto error = multibase->decode(base64_with_whitespaces);

  ASSERT_FALSE(error.has_value());
}

class Base64Encoding : public MultibaseCodecTest {
 public:
  MultibaseCodec::Encoding encoding = MultibaseCodec::Encoding::BASE64;

  const std::vector<std::pair<ByteArray, std::string_view>> decode_encode_table{
      {"66"_unhex, "mZg=="},
      {"666f"_unhex, "mZm8="},
      {"666f6f"_unhex, "mZm9v"},
      {"666f6f62"_unhex, "mZm9vYg=="},
      {"666f6f6261"_unhex, "mZm9vYmE="},
      {"666f6f626172"_unhex, "mZm9vYmFy"},
      {"4d616e2069732064697374696e677569736865642c206e6f74206f6e6c7920627"
       "92068697320726561736f6e2c2062757420627920746869732073696e67756c61"
       "722070617373696f6e2066726f6d206f7468657220616e696d616c732c2077686"
       "963682069732061206c757374206f6620746865206d696e642c20746861742062"
       "792061207065727365766572616e6365206f662064656c6967687420696e20746"
       "86520636f6e74696e75656420616e6420696e6465666174696761626c65206765"
       "6e65726174696f6e206f66206b6e6f776c656467652c206578636565647320746"
       "8652073686f727420766568656d656e6365206f6620616e79206361726e616c20"
       "706c6561737572652e"_unhex,
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
