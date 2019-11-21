/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/crypto_provider/crypto_provider_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <gsl/gsl_util>
#include "libp2p/crypto/error.hpp"
#include "libp2p/crypto/random_generator/boost_generator.hpp"
#include "testutil/outcome.hpp"

using libp2p::common::ByteArray;
using libp2p::crypto::CryptoProviderImpl;
using libp2p::crypto::Key;
using libp2p::crypto::KeyGeneratorError;
using libp2p::crypto::random::BoostRandomGenerator;

class KeyGeneratorTest : public ::testing::TestWithParam<Key::Type> {
 protected:
  BoostRandomGenerator random_;
  CryptoProviderImpl crypto_provider_{random_};
};

/**
 * @given key generator and key type as parameter
 * @when generateKeys is called
 * @then key pair of specified type successfully generated
 */
TEST_P(KeyGeneratorTest, GenerateKeyPairSuccess) {
  auto key_type = GetParam();
  if (key_type == Key::Type::RSA) {
    return;
  }

  EXPECT_OUTCOME_TRUE_2(val, crypto_provider_.generateKeys(key_type))
  ASSERT_EQ(val.privateKey.type, key_type);
  ASSERT_EQ(val.publicKey.type, key_type);
}

/**
 * @given key generator and key type as parameter
 * @when generateKeys is 2 times sequentially called
 * @then generated key pairs are different
 */
TEST_P(KeyGeneratorTest, TwoKeysAreDifferent) {
  auto key_type = GetParam();
  if (key_type == Key::Type::RSA) {
    return;
  }

  EXPECT_OUTCOME_TRUE_2(val1, crypto_provider_.generateKeys(key_type));
  EXPECT_OUTCOME_TRUE_2(val2, crypto_provider_.generateKeys(key_type));
  ASSERT_NE(val1.privateKey.data, val2.privateKey.data);
  ASSERT_NE(val1.publicKey.data, val2.privateKey.data);
}

/**
 * @given key generator and key type as parameter
 * @when call to generateKeys is successful
 * and then derivePublicKey with generated private key as parameter is called
 * @then derived public key is successfully generated
 * and is equal to the generated one
 */
TEST_P(KeyGeneratorTest, DerivePublicKeySuccess) {
  auto key_type = GetParam();
  if (key_type == Key::Type::RSA) {
    return;
  }

  EXPECT_OUTCOME_TRUE_2(keys, crypto_provider_.generateKeys(key_type));
  EXPECT_OUTCOME_TRUE_2(derived,
                        crypto_provider_.derivePublicKey(keys.privateKey));
  ASSERT_EQ(derived.type, key_type);
  ASSERT_EQ(keys.publicKey.data, derived.data);
}

INSTANTIATE_TEST_CASE_P(TestAllKeyTypes, KeyGeneratorTest,
                        ::testing::Values(Key::Type::RSA, Key::Type::Ed25519,
                                          Key::Type::Secp256k1));

class KeyLengthTest
    : public ::testing::TestWithParam<
          std::tuple<Key::Type, const uint32_t, const uint32_t>> {
 protected:
  BoostRandomGenerator random_;
  CryptoProviderImpl keygen_{random_};
};

INSTANTIATE_TEST_CASE_P(
    TestSomeKeyLengths, KeyLengthTest,
    ::testing::Values(std::tuple(Key::Type::Ed25519, 32, 32),
                      std::tuple(Key::Type::Secp256k1, 32, 33)));

/**
 * @given key generator and tuple of <key type, private key length, public key
 * length>
 * @when key pair is generated
 * @then public and private key lengths are equal to those in parameters
 */
TEST_P(KeyLengthTest, KeyLengthCorrect) {
  auto [key_type, private_key_length, public_key_length] = GetParam();

  EXPECT_OUTCOME_TRUE_2(val, keygen_.generateKeys(key_type))
  ASSERT_EQ(val.privateKey.data.size(), private_key_length);
}
