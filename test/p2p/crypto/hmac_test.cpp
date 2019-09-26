/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/hmac_provider/hmac_provider_impl.hpp"

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>
#include "testutil/literals.hpp"
#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"
#include "libp2p/crypto/error.hpp"

using kagome::common::Buffer;
using namespace libp2p::crypto;

class HmacTest : public testing::Test {
 protected:
  void SetUp() override {
    // use the same message for all tests
    message.put("The fly got to the jam that's all the poem");
  }

  /// hash provider
  hmac::HmacProviderImpl provider;
  /// message to be hashed
  Buffer message;

  Buffer sha1_key{"55cd433be9568ee79525a0919cf4b31c28108cee"_unhex};  // 20 bytes

  Buffer sha256_key{
      "a1990aeb68efb1b59d3165795f6338960aa7238ba74779ea5df3a435fdbb8d4c"_unhex};  // 32 bytes

  Buffer sha512_key{
      // 64 bytes
      "dd114c7351b2186aeba2d3fb4d96496da9e1681ae6272df553a8235a05e6f1ae"
      "66d5c4efa32cdfbf1b0f3b9542c14444a523859cde43736c7b5b899803d1a96a"_unhex};

  Buffer sha1_dgst{
      "42985601b3d61125e02bcca5a4dcb9e3763bc942"_unhex};  // 20 bytes

  Buffer sha256_dgst{
      "bdb5a9c8f3e08fdb8c0ee7189d76fd6c487d5789e0141850bcc945558488097a"_unhex};  // 32 bytes

  Buffer sha512_dgst{
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
 * @given invalid HashType value
 * @when hmacDigest is applied with given value
 * @then error is returned
 */
TEST_F(HmacTest, HashInvalidFails) {
  auto &&digest = provider.calculateDigest(static_cast<common::HashType>(15),
                                           sha1_key, message);
  ASSERT_FALSE(digest);
  ASSERT_EQ(digest.error().value(),
            static_cast<int>(HmacProviderError::UNSUPPORTED_HASH_METHOD));
}
