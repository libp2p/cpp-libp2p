/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/crypto_provider/crypto_provider_impl.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <gsl/gsl_util>
#include <libp2p/common/literals.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <testutil/outcome.hpp>

using libp2p::common::ByteArray;
using libp2p::crypto::CryptoProvider;
using libp2p::crypto::CryptoProviderImpl;
using libp2p::crypto::Key;
using libp2p::crypto::KeyGeneratorError;
using libp2p::crypto::PrivateKey;
using libp2p::crypto::ed25519::Ed25519Provider;
using libp2p::crypto::ed25519::Ed25519ProviderImpl;
using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::random::CSPRNG;
using libp2p::common::operator""_unhex;
using libp2p::common::operator""_v;

class KeyGeneratorTest : public ::testing::TestWithParam<Key::Type> {
 public:
  KeyGeneratorTest()
      : random_{std::make_shared<BoostRandomGenerator>()},
        ed25519_provider_{std::make_shared<Ed25519ProviderImpl>()},
        crypto_provider_{
            std::make_shared<CryptoProviderImpl>(random_, ed25519_provider_)} {}

 protected:
  std::shared_ptr<CSPRNG> random_;
  std::shared_ptr<Ed25519Provider> ed25519_provider_;
  std::shared_ptr<CryptoProvider> crypto_provider_;
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

  EXPECT_OUTCOME_TRUE_2(val, crypto_provider_->generateKeys(key_type))
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

  EXPECT_OUTCOME_TRUE_2(val1, crypto_provider_->generateKeys(key_type));
  EXPECT_OUTCOME_TRUE_2(val2, crypto_provider_->generateKeys(key_type));
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

  EXPECT_OUTCOME_TRUE_2(keys, crypto_provider_->generateKeys(key_type));
  EXPECT_OUTCOME_TRUE_2(derived,
                        crypto_provider_->derivePublicKey(keys.privateKey));
  ASSERT_EQ(derived.type, key_type);
  ASSERT_EQ(keys.publicKey.data, derived.data);
}

INSTANTIATE_TEST_CASE_P(TestAllKeyTypes, KeyGeneratorTest,
                        ::testing::Values(Key::Type::RSA, Key::Type::Ed25519,
                                          Key::Type::Secp256k1,
                                          Key::Type::ECDSA));

class KeyLengthTest
    : public ::testing::TestWithParam<
          std::tuple<Key::Type, const uint32_t, const uint32_t>> {
 public:
  KeyLengthTest()
      : random_{std::make_shared<BoostRandomGenerator>()},
        ed25519_provider_{std::make_shared<Ed25519ProviderImpl>()},
        crypto_provider_{
            std::make_shared<CryptoProviderImpl>(random_, ed25519_provider_)} {}

 protected:
  std::shared_ptr<CSPRNG> random_;
  std::shared_ptr<Ed25519Provider> ed25519_provider_;
  std::shared_ptr<CryptoProvider> crypto_provider_;
};

INSTANTIATE_TEST_CASE_P(
    TestSomeKeyLengths, KeyLengthTest,
    ::testing::Values(std::tuple(Key::Type::Ed25519, 32, 32),
                      std::tuple(Key::Type::Secp256k1, 32, 33),
                      std::tuple(Key::Type::ECDSA, 32, 33)));

/**
 * @given key generator and tuple of <key type, private key length, public key
 * length>
 * @when key pair is generated
 * @then public and private key lengths are equal to those in parameters
 */
TEST_P(KeyLengthTest, KeyLengthCorrect) {
  auto [key_type, private_key_length, public_key_length] = GetParam();

  EXPECT_OUTCOME_TRUE_2(val, crypto_provider_->generateKeys(key_type))
  ASSERT_EQ(val.privateKey.data.size(), private_key_length);
}

// TODO(turuslan): convert to TestWithParam and test more key types
class KeyGoCompatibility : public ::testing::Test {
 public:
  KeyGoCompatibility()
      : random_{std::make_shared<BoostRandomGenerator>()},
        ed25519_provider_{std::make_shared<Ed25519ProviderImpl>()},
        crypto_provider_{
            std::make_shared<CryptoProviderImpl>(random_, ed25519_provider_)} {}

 protected:
  std::shared_ptr<CSPRNG> random_;
  std::shared_ptr<Ed25519Provider> ed25519_provider_;
  std::shared_ptr<CryptoProvider> crypto_provider_;
};

TEST_F(KeyGoCompatibility, ECDSA) {
  PrivateKey privateKey{
      {Key::Type::ECDSA,
       "325153c93c647c8b7645f69f5a91aba5bb0a0c6c9256264a3cd7f860e9916c28"_unhex}};

  auto derivedPublicKey = crypto_provider_->derivePublicKey(privateKey).value();

  EXPECT_EQ(
      derivedPublicKey.data,
      "033571844d75a74a49a3b5e2953261078ff60cacd270cab134c8b70ded6d26e5cd"_unhex);
}

TEST_F(KeyGoCompatibility, Ed25519) {
  PrivateKey private_key{
      {Key::Type::Ed25519,
       "6d8e72d53e0f8582f52169bf7f6c60ddb7e0fbb83af97a11cff02f1bf21bbf7c"_unhex}};

  auto derived = crypto_provider_->derivePublicKey(private_key).value();
  EXPECT_EQ(
      derived.data,
      "821dc9f866442249e26985c7fadca424de7df4534f50383bec9a92f538a2063b"_unhex);

  auto message{"think of the rapture!"_v};
  const size_t message_len{21};  // here we do not count terminating null char
  ASSERT_EQ(message.size(), message_len);

  auto msg_span = gsl::make_span(message.data(), message_len);
  auto signature = crypto_provider_->sign(msg_span, private_key).value();
  ASSERT_EQ(signature.size(), 64);
  EXPECT_EQ(
      signature,
      "575304fbd0f8096439ca18e588beffc67218e3d117a14cb41cecf3bc180f9496"
      "90e5be626ae678a23ac5dfcccc516acc0527f67e0f0a696525a31d667305d406"_unhex);

  auto verify_result =
      crypto_provider_->verify(msg_span, signature, derived).value();
  ASSERT_TRUE(verify_result);
}
