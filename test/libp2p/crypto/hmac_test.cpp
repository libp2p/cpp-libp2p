/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/hmac_provider/hmac_provider_ctr_impl.hpp>
#include <libp2p/crypto/hmac_provider/hmac_provider_impl.hpp>

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/error.hpp>
#include <libp2p/outcome/outcome.hpp>

using namespace libp2p::crypto;
using namespace libp2p::common;

class HmacTest : public testing::Test {
 protected:
  void SetUp() override {
    // use the same message for all tests
    std::string m = "The fly got to the jam that's all the poem";
    message.insert(message.end(), m.begin(), m.end());
  }

  /// hash provider
  hmac::HmacProviderImpl provider;
  /// message to be hashed
  ByteArray message;

  ByteArray sha1_key{
      "55cd433be9568ee79525a0919cf4b31c28108cee"_unhex};  // 20 bytes

  ByteArray sha256_key{
      "a1990aeb68efb1b59d3165795f6338960aa7238ba74779ea5df3a435fdbb8d4c"_unhex};  // 32 bytes

  ByteArray sha512_key{
      // 64 bytes
      "dd114c7351b2186aeba2d3fb4d96496da9e1681ae6272df553a8235a05e6f1ae"
      "66d5c4efa32cdfbf1b0f3b9542c14444a523859cde43736c7b5b899803d1a96a"_unhex};

  ByteArray sha1_dgst{
      "42985601b3d61125e02bcca5a4dcb9e3763bc942"_unhex};  // 20 bytes

  ByteArray sha256_dgst{
      "bdb5a9c8f3e08fdb8c0ee7189d76fd6c487d5789e0141850bcc945558488097a"_unhex};  // 32 bytes

  ByteArray sha512_dgst{
      "0f5bf6af4943b35b76d7d89714b681900e03262e997f2519befd7b1cb0cb56e8"
      "e648fa297ba1855382123240f6cded44174b851b94665b9a56b249d4d88deb63"_unhex};  // 64 bytes
};

/**
 * @given 20 bytes key, default message
 * @when hmacDigest is applied with hash = kSha1
 * @then obtained digest matches predefined one
 */
TEST_F(HmacTest, HashSha1Success) {
  auto &&digest =
      provider.calculateDigest(common::HashType::SHA1, sha1_key, message);
  ASSERT_TRUE(digest);
  ASSERT_EQ(digest.value().size(), 20);
  ASSERT_EQ(digest.value(), sha1_dgst);
}

/**
 * @given 32 bytes key, default message
 * @when hmacDigest is applied with hash = kSha256
 * @then obtained digest matches predefined one
 */
TEST_F(HmacTest, HashSha256Success) {
  auto &&digest =
      provider.calculateDigest(common::HashType::SHA256, sha256_key, message);
  ASSERT_TRUE(digest);
  ASSERT_EQ(digest.value().size(), 32);
  ASSERT_EQ(digest.value(), sha256_dgst);
}

/**
 * @given 64 bytes key, default message
 * @when hmacDigest is applied with hash = kSha512
 * @then obtained digest matches predefined one
 */
TEST_F(HmacTest, HashSha512Success) {
  auto &&digest =
      provider.calculateDigest(common::HashType::SHA512, sha512_key, message);
  ASSERT_TRUE(digest);
  ASSERT_EQ(digest.value().size(), 64);
  ASSERT_EQ(digest.value(), sha512_dgst);
}

/**
 * @given initialized HMAC instance
 * @when digest gets calculated and HMAC is reset
 * @then the state was correctly reset and the same calculation gives the same
 * result
 */
TEST_F(HmacTest, HmacCtrTest) {
  hmac::HmacProviderCtrImpl hmac{common::HashType::SHA256, sha256_key};
  ASSERT_TRUE(hmac.write(message));
  auto &&digest = hmac.digest();
  ASSERT_TRUE(digest);
  ASSERT_EQ(digest.value(), sha256_dgst);
  ASSERT_TRUE(hmac.reset());
  ASSERT_TRUE(hmac.write(message));
  auto &&digest2 = hmac.digest();
  ASSERT_TRUE(digest2);
  ASSERT_EQ(digest2.value(), sha256_dgst);
}
