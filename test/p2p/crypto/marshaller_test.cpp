/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_marshaller/key_marshaller_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "libp2p/crypto/common.hpp"
#include "mock/libp2p/crypto/key_validator_mock.hpp"

using Buffer = std::vector<uint8_t>;
using libp2p::crypto::Key;
using libp2p::crypto::PrivateKey;
using libp2p::crypto::PublicKey;
using libp2p::crypto::marshaller::KeyMarshallerImpl;
using libp2p::crypto::validator::KeyValidator;
using libp2p::crypto::validator::KeyValidatorMock;

using ::testing::_;
using ::testing::An;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

template <typename T>
struct KeyCase {
  T key;
  Buffer match;
};

Buffer randomBuffer(size_t size) {
  Buffer buf(size, 0);
  std::generate_n(buf.begin(), size, []() { return rand() & 0xff; });
  return buf;
}

class Pubkey : public testing::TestWithParam<KeyCase<PublicKey>> {
 public:
  void SetUp() override {
    ON_CALL(*validator_, validate(::testing::An<const PublicKey &>()))
        .WillByDefault(Return(outcome::success()));
  }
  std::shared_ptr<KeyValidatorMock> validator_ =
      std::make_shared<KeyValidatorMock>();
  KeyMarshallerImpl marshaller_{validator_};
};

class Privkey : public testing::TestWithParam<KeyCase<PrivateKey>> {
 public:
  void SetUp() override {
    ON_CALL(*validator_, validate(::testing::An<const PrivateKey &>()))
        .WillByDefault(Return(outcome::success()));
  }

  std::shared_ptr<KeyValidatorMock> validator_ =
      std::make_shared<KeyValidatorMock>();
  KeyMarshallerImpl marshaller_{validator_};
};

TEST_P(Pubkey, Valid) {
  auto [key, match_prefix] = GetParam();

  Buffer match;
  match.insert(match.begin(), match_prefix.begin(), match_prefix.end());
  match.insert(match.end(), key.data.begin(), key.data.end());

  {
    auto &&res = marshaller_.marshal(key);
    ASSERT_TRUE(res);
    auto &&val = res.value();

    ASSERT_EQ(val, match);
  }
  {
    auto &&res = marshaller_.unmarshalPublicKey(match);
    ASSERT_TRUE(res);
    auto &&val = res.value();
    ASSERT_EQ(val.type, key.type);
    ASSERT_EQ(val.data, key.data);
  }
}

TEST_P(Privkey, Valid) {
  auto [key, match_prefix] = GetParam();

  Buffer match;
  match.insert(match.begin(), match_prefix.begin(), match_prefix.end());
  match.insert(match.end(), key.data.begin(), key.data.end());

  {
    auto &&res = marshaller_.marshal(key);
    ASSERT_TRUE(res);
    auto &&val = res.value();

    ASSERT_EQ(val, match);
  }
  {
    auto &&res = marshaller_.unmarshalPrivateKey(match);
    ASSERT_TRUE(res);
    auto &&val = res.value();
    ASSERT_EQ(val.type, key.type);
    ASSERT_EQ(val.data, key.data);
  }
}

template <typename T>
auto makeTestCases() {
  // clang-format off
  return std::vector<KeyCase<T>>{
      {{T{{Key::Type::UNSPECIFIED, randomBuffer(16)}}}, {18, 16}},
      {{T{{Key::Type::RSA1024,     randomBuffer(16)}}}, {8, 1, 18, 16}},
      {{T{{Key::Type::RSA2048,     randomBuffer(16)}}}, {8, 2, 18, 16}},
      {{T{{Key::Type::RSA4096,     randomBuffer(16)}}}, {8, 3, 18, 16}},
      {{T{{Key::Type::ED25519,     randomBuffer(16)}}}, {8, 4, 18, 16}},
      {{T{{Key::Type::SECP256K1,   randomBuffer(16)}}}, {8, 5, 18, 16}},
  };
  // clang-format on
}

INSTANTIATE_TEST_CASE_P(Marshaller, Pubkey,
                        ::testing::ValuesIn(makeTestCases<PublicKey>()));

INSTANTIATE_TEST_CASE_P(Marshaller, Privkey,
                        ::testing::ValuesIn(makeTestCases<PrivateKey>()));
