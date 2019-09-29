/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/outcome/outcome.hpp>
#include "libp2p/crypto/common.hpp"
#include "libp2p/crypto/key.hpp"

using namespace libp2p::crypto;

/**
 * @given sequence of bytes
 * @when private key of unspecified type containing provided bytes is created
 * @then created key contains provided sequence of bytes and it's type is
 * unspecified
 */
TEST(PrivateKeyTest, KeyCreateSuccess) {
  auto key = PrivateKey{{Key::Type::UNSPECIFIED, {1, 2, 3}}};
  ASSERT_EQ(key.data, (std::vector<uint8_t>{1, 2, 3}));
  ASSERT_EQ(key.type, Key::Type::UNSPECIFIED);
}

/**
 * @given sequence of bytes
 * @when public key of unspecified type containing provided bytes is created
 * @then created key contains provided sequence of bytes and it's type is
 * unspecified
 */
TEST(PublicKeyTest, KeyCreateSuccess) {
  auto key = PublicKey{{Key::Type::UNSPECIFIED, {1, 2, 3}}};
  ASSERT_EQ(key.data, (std::vector<uint8_t>{1, 2, 3}));
  ASSERT_EQ(key.type, Key::Type::UNSPECIFIED);
}
