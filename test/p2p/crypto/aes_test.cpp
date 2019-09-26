/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "libp2p/crypto/aes_provider/aes_provider_impl.hpp"

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"
#include "testutil/literals.hpp"

using kagome::common::Buffer;
using namespace libp2p::crypto;

class AesTest : public testing::Test {
 protected:
  using Aes128Secret = libp2p::crypto::common::Aes128Secret;
  using Aes256Secret = libp2p::crypto::common::Aes256Secret;

  void SetUp() override {
    plain_text_128.put("Single block msg");
    plain_text_256.put("The fly got to the jam that's all the poem");
  }

  Buffer iv{"3dafba429d9eb430b422da802c9fac41"_unhex};

  Buffer key_128{"06a9214036b8a15b512e03d534120006"_unhex};

  Buffer key_256{
      "78dae34bc0eba813c09cec5c871f3ccb39dcbbe04a2fe1837e169fee896aa208"_unhex};

  Buffer cipher_text_128{"d43130f652c4c81be62fdf5e72e48cbc"_unhex};

  Buffer cipher_text_256{
      "586a49b4ba0336ffe130c5f27b80d3c9910d7f422687a60b1b833cff3d9ecbe03e4db5653a671fb1a7b2"_unhex};

  Buffer plain_text_128;
  Buffer plain_text_256;

  aes::AesProviderImpl provider;
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

  auto &&result = provider.encryptAesCtr128(secret, plain_text_128);
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

  auto &&result = provider.encryptAesCtr256(secret, plain_text_256);  // NOLINT
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

  auto &&result = provider.decryptAesCtr128(secret, cipher_text_128);
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

  auto &&result = provider.decryptAesCtr256(secret, cipher_text_256);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), plain_text_256);
}
