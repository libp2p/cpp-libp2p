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
#include <libp2p/crypto/hmac_provider/hmac_provider_impl.hpp>
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
using libp2p::crypto::hmac::HmacProvider;
using libp2p::crypto::hmac::HmacProviderImpl;
using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::random::CSPRNG;
using libp2p::common::operator""_unhex;
using libp2p::common::operator""_v;

class KeyGeneratorTest : public ::testing::TestWithParam<Key::Type> {
 public:
  KeyGeneratorTest()
      : random_{std::make_shared<BoostRandomGenerator>()},
        ed25519_provider_{std::make_shared<Ed25519ProviderImpl>()},
        hmac_provider_{std::make_shared<HmacProviderImpl>()},
        crypto_provider_{std::make_shared<CryptoProviderImpl>(
            random_, ed25519_provider_, hmac_provider_)} {}

 protected:
  std::shared_ptr<CSPRNG> random_;
  std::shared_ptr<Ed25519Provider> ed25519_provider_;
  std::shared_ptr<HmacProvider> hmac_provider_;
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
        hmac_provider_{std::make_shared<HmacProviderImpl>()},
        crypto_provider_{std::make_shared<CryptoProviderImpl>(
            random_, ed25519_provider_, hmac_provider_)} {}

 protected:
  std::shared_ptr<CSPRNG> random_;
  std::shared_ptr<Ed25519Provider> ed25519_provider_;
  std::shared_ptr<HmacProvider> hmac_provider_;
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
        hmac_provider_{std::make_shared<HmacProviderImpl>()},
        crypto_provider_{std::make_shared<CryptoProviderImpl>(
            random_, ed25519_provider_, hmac_provider_)} {}

 protected:
  std::shared_ptr<CSPRNG> random_;
  std::shared_ptr<Ed25519Provider> ed25519_provider_;
  std::shared_ptr<HmacProvider> hmac_provider_;
  std::shared_ptr<CryptoProvider> crypto_provider_;
};

TEST_F(KeyGoCompatibility, RSA) {
  PrivateKey privateKey{
      {Key::Type::RSA,
       "3082025e02010002818100bcaf3ee0f2bc3ac58ab3fcb3c23b2386230564331653ae34e9e09ea6fb0b9cfcf9c6ef76c9337d9b8ed29b4505c8e57a06a9a008ecb89ece3a6e809af64342be4367e06ba1bec131c8944465ba1f5cead836e84932097aea1f6aefc97e84f76219b9dec8afd7a1d0fa90802bd84b1d021112daf026c60ad958db4247e56dc39d0203010001028180407fdb8bc40e6a3ccafc59ff0cff705653346d9b351fa7e678a88b33639005bb489b2392c496b07273b134d8b47087595e5bafd43d2fa341b621be1ebade253b149ba6df498b94269b708547406aeb5da7d71e4fa52fff331cfbae3db55c51ed896d914e93bc0a703aaafed6fe83e7f9af20c2fcfd7207d34426b6b4ed8283b5024100e678fb31b2489505bdb0cf16c23fd6e4ff5069de71f72c12e5a1b0c295aa4fa6e2b691fd5c5ea98473d0884dd969a258f48e5593bdc15f8c72f9da775ca0aeff024100d1955ed85222b96e55e1f9d7865dcb78467a12839f0a3f5b17791b15a1d5b14c20e96bb6d352988f628030282a1c44027e168bb79dac7eb858c1bb3c6ffce963024100d298a808203dfc96336055cb1912d69d87c3060a729f0651fa2cc664f7f7993308a5053fbb60f08b8c7c77a09352d83b6ab488f428878374c63712eed0e02f27024100b94ed2ef7da00a488e5321aef8b511e4a49be6a6ce062782893ca13ffd398e6bfb65a7c19d1398a49eb92cdb36708b8990a6aa9e8d21296221c8199f147d9075024100a7abd450a0c8fe8f3cb2c0d8fca3f15094b512dce328ce543977c14f80dcb7e41ac4ae7fb2925fae724b6e2494231d0c51572ae89510b4ce6e984623ddf2c923"_unhex}};

  auto derivedPublicKey = crypto_provider_->derivePublicKey(privateKey).value();

  EXPECT_EQ(
      derivedPublicKey.data,
      "30819f300d06092a864886f70d010101050003818d0030818902818100bcaf3ee0f2bc3ac58ab3fcb3c23b2386230564331653ae34e9e09ea6fb0b9cfcf9c6ef76c9337d9b8ed29b4505c8e57a06a9a008ecb89ece3a6e809af64342be4367e06ba1bec131c8944465ba1f5cead836e84932097aea1f6aefc97e84f76219b9dec8afd7a1d0fa90802bd84b1d021112daf026c60ad958db4247e56dc39d0203010001"_unhex);
}

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
