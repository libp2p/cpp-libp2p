/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "libp2p/crypto/aes_ctr/aes_ctr_impl.hpp"

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/outcome/outcome.hpp>

using namespace libp2p::crypto;
using namespace libp2p::common;

class AesTest : public testing::Test {
 protected:
  using Aes128Secret = libp2p::crypto::common::Aes128Secret;
  using Aes256Secret = libp2p::crypto::common::Aes256Secret;

  void SetUp() override {
    std::string_view msg1 = "Single block msg";
    plain_text_128.insert(plain_text_128.end(), msg1.begin(), msg1.end());
    std::string_view msg2 = "The fly got to the jam that's all the poem";
    plain_text_256.insert(plain_text_256.end(), msg2.begin(), msg2.end());
  }

  ByteArray iv{"3dafba429d9eb430b422da802c9fac41"_unhex};

  ByteArray key_128{"06a9214036b8a15b512e03d534120006"_unhex};

  ByteArray key_256{
      "78dae34bc0eba813c09cec5c871f3ccb39dcbbe04a2fe1837e169fee896aa208"_unhex};

  ByteArray cipher_text_128{"d43130f652c4c81be62fdf5e72e48cbc"_unhex};

  ByteArray cipher_text_256{
      "586a49b4ba0336ffe130c5f27b80d3c9910d7f422687a60b1b833cff3d9ecbe03e4db5653a671fb1a7b2"_unhex};

  ByteArray plain_text_128;
  ByteArray plain_text_256;
};

/**
 * @given key, iv, plain text and encrypted text
 * @when encrypt aes-128-ctr is applied
 * @then result matches encrypted text
 */
TEST_F(AesTest, EncodeAesCtr128Success) {
  Aes128Secret secret{};

  std::copy(key_128.begin(), key_128.end(), secret.key.begin());
  std::copy(iv.begin(), iv.end(), secret.iv.begin());

  auto &&result = aes::AesCtrImpl(secret, aes::AesCtrImpl::Mode::ENCRYPT)
                      .crypt(plain_text_128);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), cipher_text_128);
}

/**
 * @given key, iv, plain text and encrypted text
 * @when encrypt aes-256-ctr is applied
 * @then result matches encrypted text
 */
TEST_F(AesTest, EncodeAesCtr256Success) {
  Aes256Secret secret{};

  std::copy(key_256.begin(), key_256.end(), secret.key.begin());
  std::copy(iv.begin(), iv.end(), secret.iv.begin());

  auto &&result = aes::AesCtrImpl(secret, aes::AesCtrImpl::Mode::ENCRYPT)
                      .crypt(plain_text_256);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), cipher_text_256);
}

/**
 * @given key, iv, plain text and encrypted text
 * @when decrypt aes-128-ctr is applied
 * @then result matches plain text
 */
TEST_F(AesTest, DecodeAesCtr128Success) {
  Aes128Secret secret{};

  std::copy(key_128.begin(), key_128.end(), secret.key.begin());
  std::copy(iv.begin(), iv.end(), secret.iv.begin());

  auto &&result = aes::AesCtrImpl(secret, aes::AesCtrImpl::Mode::DECRYPT)
                      .crypt(cipher_text_128);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), plain_text_128);
}

/**
 * @given key, iv, plain text and encrypted text
 * @when decrypt aes-256-ctr is applied
 * @then result matches plain text
 */
TEST_F(AesTest, DecodeAesCtr256Success) {
  Aes256Secret secret{};

  std::copy(key_256.begin(), key_256.end(), secret.key.begin());
  std::copy(iv.begin(), iv.end(), secret.iv.begin());

  auto &&result = aes::AesCtrImpl(secret, aes::AesCtrImpl::Mode::DECRYPT)
                      .crypt(cipher_text_256);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), plain_text_256);
}

/**
 * @given two identical encrypted streams
 * @when one stream is decrypted at once and the second in two approaches
 * @then the resulting decrypted data is equal and valid
 */
TEST_F(AesTest, Stream) {
  Aes256Secret secret{};

  std::copy(key_256.begin(), key_256.end(), secret.key.begin());
  std::copy(iv.begin(), iv.end(), secret.iv.begin());

  auto cipher_text = gsl::make_span(cipher_text_256);
  const auto kDelimiter = 20;
  auto cipher_text_part_1 = cipher_text.subspan(0, kDelimiter);
  auto cipher_text_part_2 =
      cipher_text.subspan(kDelimiter, cipher_text.size() - kDelimiter);

  auto &&result_ref = aes::AesCtrImpl(secret, aes::AesCtrImpl::Mode::DECRYPT)
                          .crypt(cipher_text_256);
  ASSERT_TRUE(result_ref);
  ASSERT_EQ(result_ref.value(), plain_text_256);

  aes::AesCtrImpl ctr(secret, aes::AesCtrImpl::Mode::DECRYPT);
  auto &&result_part_1 = ctr.crypt(cipher_text_part_1);
  auto &&result_part_2 = ctr.crypt(cipher_text_part_2);
  ASSERT_TRUE(result_part_1);
  ASSERT_TRUE(result_part_2);
  ASSERT_EQ(plain_text_256.size(),
            result_part_1.value().size() + result_part_2.value().size());
  ByteArray out;
  out.insert(out.end(), result_part_1.value().begin(),
             result_part_1.value().end());
  out.insert(out.end(), result_part_2.value().begin(),
             result_part_2.value().end());
  ASSERT_EQ(plain_text_256, out);
}
