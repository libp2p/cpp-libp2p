/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_validator/key_validator_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <libp2p/crypto/crypto_provider/crypto_provider_impl.hpp>
#include <libp2p/crypto/ecdsa_provider/ecdsa_provider_impl.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/hmac_provider/hmac_provider_impl.hpp>
#include <libp2p/crypto/key_validator/key_validator_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/crypto/rsa_provider/rsa_provider_impl.hpp>
#include <libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp>
#include <testutil/outcome.hpp>

using ::testing::_;
using ::testing::An;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

using libp2p::crypto::CryptoProvider;
using libp2p::crypto::CryptoProviderImpl;
using libp2p::crypto::Key;
using libp2p::crypto::KeyPair;
using libp2p::crypto::PrivateKey;
using libp2p::crypto::PublicKey;
using libp2p::crypto::ecdsa::EcdsaProvider;
using libp2p::crypto::ecdsa::EcdsaProviderImpl;
using libp2p::crypto::ed25519::Ed25519Provider;
using libp2p::crypto::ed25519::Ed25519ProviderImpl;
using libp2p::crypto::hmac::HmacProvider;
using libp2p::crypto::hmac::HmacProviderImpl;
using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::random::CSPRNG;
using libp2p::crypto::rsa::RsaProvider;
using libp2p::crypto::rsa::RsaProviderImpl;
using libp2p::crypto::secp256k1::Secp256k1Provider;
using libp2p::crypto::secp256k1::Secp256k1ProviderImpl;
using libp2p::crypto::validator::KeyValidator;
using libp2p::crypto::validator::KeyValidatorImpl;

struct BaseKeyTest {
  std::shared_ptr<CSPRNG> random = std::make_shared<BoostRandomGenerator>();
  std::shared_ptr<Ed25519Provider> ed25519 =
      std::make_shared<Ed25519ProviderImpl>();
  std::shared_ptr<RsaProvider> rsa = std::make_shared<RsaProviderImpl>();
  std::shared_ptr<EcdsaProvider> ecdsa = std::make_shared<EcdsaProviderImpl>();
  std::shared_ptr<Secp256k1Provider> secp256k1 =
      std::make_shared<Secp256k1ProviderImpl>();
  std::shared_ptr<HmacProvider> hmac_provider =
      std::make_shared<HmacProviderImpl>();
  std::shared_ptr<CryptoProvider> crypto_provider =
      std::make_shared<CryptoProviderImpl>(random, ed25519, rsa, ecdsa,
                                           secp256k1, hmac_provider);
  std::shared_ptr<KeyValidator> validator =
      std::make_shared<KeyValidatorImpl>(crypto_provider);
};

class GeneratedKeysTest : public BaseKeyTest,
                          public ::testing::TestWithParam<Key::Type> {};

/**
 * Generated keys are always valid
 */

/**
 * @given key type as parameter
 * @when generate arbitrary key pair using generator
 * @and validate public, private keys and key pair using validator
 * @then result of validation is success
 */
TEST_P(GeneratedKeysTest, GeneratedKeysAreValid) {
  Key::Type key_type = GetParam();
  EXPECT_OUTCOME_TRUE(key_pair, crypto_provider->generateKeys(key_type))
  EXPECT_OUTCOME_TRUE_1(validator->validate(key_pair.publicKey))
  EXPECT_OUTCOME_TRUE_1(validator->validate(key_pair.privateKey))
  EXPECT_OUTCOME_TRUE_1(validator->validate(key_pair))
}

/**
 * Arbitrary composed keys are not valid, used to show that
 * not everything is successfully validated
 */

/**
 * @given key type as parameter
 * @when compose private and public keys as random sequences
 * @and validate it using validator
 * @then result of validation is failure
 */
TEST_P(GeneratedKeysTest, ArbitraryKeyInvalid) {
  Key::Type key_type = GetParam();
  auto public_key = PublicKey{{key_type, random->randomBytes(64)}};
  EXPECT_OUTCOME_FALSE_1(validator->validate(public_key))

  auto private_key = PrivateKey{{key_type, random->randomBytes(64)}};
  EXPECT_OUTCOME_FALSE_1(validator->validate(private_key))
}

/**
 * When public key in a generated pair is replaced by invalid
 * key pair becomes invalid
 */

/**
 * @given key type as parameter
 * @when generate key pair
 * @and replace public key by invalid one @and validate new pair
 * @then validation result of new key pair is failure
 */
TEST_P(GeneratedKeysTest, InvalidPublicKeyInvalidatesPair) {
  Key::Type key_type = GetParam();

  EXPECT_OUTCOME_TRUE(key_pair, crypto_provider->generateKeys(key_type))
  auto public_key = PublicKey{{key_type, random->randomBytes(64)}};
  EXPECT_OUTCOME_FALSE_1(validator->validate(public_key))
  auto invalid_pair = KeyPair{public_key, key_pair.privateKey};
  EXPECT_OUTCOME_FALSE_1(validator->validate(invalid_pair))
}

INSTANTIATE_TEST_CASE_P(GeneratedValidKeysCases, GeneratedKeysTest,
                        ::testing::Values(Key::Type::RSA, Key::Type::Ed25519,
                                          Key::Type::Secp256k1));

class RandomKeyTest : public BaseKeyTest,
                      public ::testing::TestWithParam<Key::Type> {};

/**
 * Every 32 byte can be ED25519 or SECP256K1 private key
 */

/**
 * @given key type as parameter (ED25519 or SECP256K1)
 * @when generate arbitrary 32 bytes sequence, compose a key @and validate
 * @then composed key validation result is success,
 * @and derive operation succeeds as well
 */
TEST_P(RandomKeyTest, Every32byteIsValidPrivateKey) {
  auto key_type = GetParam();
  auto sequence = random->randomBytes(32);
  auto private_key = PrivateKey{{key_type, sequence}};
  EXPECT_OUTCOME_TRUE_1(validator->validate(private_key))
  EXPECT_OUTCOME_TRUE_1(crypto_provider->derivePublicKey(private_key))
}

INSTANTIATE_TEST_CASE_P(RandomSequencesCases, RandomKeyTest,
                        ::testing::Values(Key::Type::Ed25519,
                                          Key::Type::Secp256k1));

class UnspecifiedKeyTest : public BaseKeyTest, public ::testing::Test {};

/**
 * @given proposed key type: Key::Type::UNSPECIFIED
 * @when compose private and public keys of UNSPECIFIED type as random sequences
 * @and validate it using validator
 * @then result of validation is success
 */
TEST_F(UnspecifiedKeyTest, UnspecifiedAlwaysValid) {
  auto private_key =
      PrivateKey{{Key::Type::UNSPECIFIED, random->randomBytes(64)}};
  EXPECT_OUTCOME_TRUE_1(validator->validate(private_key))
  auto public_key =
      PublicKey{{Key::Type::UNSPECIFIED, random->randomBytes(64)}};
  EXPECT_OUTCOME_TRUE_1(validator->validate(public_key))
  EXPECT_OUTCOME_TRUE_1(validator->validate(KeyPair{public_key, private_key}));
}
