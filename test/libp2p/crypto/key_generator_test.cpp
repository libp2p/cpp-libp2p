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
#include <libp2p/crypto/ecdsa_provider/ecdsa_provider_impl.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/hmac_provider/hmac_provider_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/crypto/rsa_provider/rsa_provider_impl.hpp>
#include <libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp>
#include <testutil/outcome.hpp>

using libp2p::common::ByteArray;
using libp2p::crypto::CryptoProvider;
using libp2p::crypto::CryptoProviderImpl;
using libp2p::crypto::Key;
using libp2p::crypto::KeyGeneratorError;
using libp2p::crypto::PrivateKey;
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
using libp2p::common::operator""_unhex;
using libp2p::common::operator""_v;

class KeyGenTest {
 public:
  KeyGenTest()
      : random_{std::make_shared<BoostRandomGenerator>()},
        ed25519_provider_{std::make_shared<Ed25519ProviderImpl>()},
        rsa_provider_{std::make_shared<RsaProviderImpl>()},
        ecdsa_provider_{std::make_shared<EcdsaProviderImpl>()},
        secp256k1_provider_{std::make_shared<Secp256k1ProviderImpl>()},
        hmac_provider_{std::make_shared<HmacProviderImpl>()},
        crypto_provider_{std::make_shared<CryptoProviderImpl>(
            random_, ed25519_provider_, rsa_provider_, ecdsa_provider_,
            secp256k1_provider_, hmac_provider_)} {}

 protected:
  std::shared_ptr<CSPRNG> random_;
  std::shared_ptr<Ed25519Provider> ed25519_provider_;
  std::shared_ptr<RsaProvider> rsa_provider_;
  std::shared_ptr<EcdsaProvider> ecdsa_provider_;
  std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
  std::shared_ptr<HmacProvider> hmac_provider_;
  std::shared_ptr<CryptoProvider> crypto_provider_;
};

class KeyGeneratorTest : public KeyGenTest,
                         public ::testing::TestWithParam<Key::Type> {};

/**
 * @given key generator and key type as parameter
 * @when generateKeys is called
 * @then key pair of specified type successfully generated
 */
TEST_P(KeyGeneratorTest, GenerateKeyPairSuccess) {
  auto key_type = GetParam();
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
    : public KeyGenTest,
      public ::testing::TestWithParam<
          std::tuple<Key::Type, const uint32_t, const uint32_t>> {};

INSTANTIATE_TEST_CASE_P(
    TestSomeKeyLengths, KeyLengthTest,
    ::testing::Values(std::tuple(Key::Type::Ed25519, 32, 32),
                      std::tuple(Key::Type::Secp256k1, 32, 33),
                      std::tuple(Key::Type::ECDSA, 121, 91)));

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

// TODO(turuslan): FIL-132 convert to TestWithParam and test more key types
class KeyGoCompatibility : public KeyGenTest, public ::testing::Test {};

TEST_F(KeyGoCompatibility, RSA_old) {
  PrivateKey privateKey{
      {Key::Type::RSA,
       "3082025e02010002818100bcaf3ee0f2bc3ac58ab3fcb3c23b2386230564331653ae34e"
       "9e09ea6fb0b9cfcf9c6ef76c9337d9b8ed29b4505c8e57a06a9a008ecb89ece3a6e809a"
       "f64342be4367e06ba1bec131c8944465ba1f5cead836e84932097aea1f6aefc97e84f76"
       "219b9dec8afd7a1d0fa90802bd84b1d021112daf026c60ad958db4247e56dc39d020301"
       "0001028180407fdb8bc40e6a3ccafc59ff0cff705653346d9b351fa7e678a88b3363900"
       "5bb489b2392c496b07273b134d8b47087595e5bafd43d2fa341b621be1ebade253b149b"
       "a6df498b94269b708547406aeb5da7d71e4fa52fff331cfbae3db55c51ed896d914e93b"
       "c0a703aaafed6fe83e7f9af20c2fcfd7207d34426b6b4ed8283b5024100e678fb31b248"
       "9505bdb0cf16c23fd6e4ff5069de71f72c12e5a1b0c295aa4fa6e2b691fd5c5ea98473d"
       "0884dd969a258f48e5593bdc15f8c72f9da775ca0aeff024100d1955ed85222b96e55e1"
       "f9d7865dcb78467a12839f0a3f5b17791b15a1d5b14c20e96bb6d352988f628030282a1"
       "c44027e168bb79dac7eb858c1bb3c6ffce963024100d298a808203dfc96336055cb1912"
       "d69d87c3060a729f0651fa2cc664f7f7993308a5053fbb60f08b8c7c77a09352d83b6ab"
       "488f428878374c63712eed0e02f27024100b94ed2ef7da00a488e5321aef8b511e4a49b"
       "e6a6ce062782893ca13ffd398e6bfb65a7c19d1398a49eb92cdb36708b8990a6aa9e8d2"
       "1296221c8199f147d9075024100a7abd450a0c8fe8f3cb2c0d8fca3f15094b512dce328"
       "ce543977c14f80dcb7e41ac4ae7fb2925fae724b6e2494231d0c51572ae89510b4ce6e9"
       "84623ddf2c923"_unhex}};

  auto derivedPublicKey = crypto_provider_->derivePublicKey(privateKey).value();

  EXPECT_EQ(
      derivedPublicKey.data,
      "30819f300d06092a864886f70d010101050003818d0030818902818100bcaf3ee0"
      "f2bc3ac58ab3fcb3c23b2386230564331653ae34e9e09ea6fb0b9cfcf9c6ef76c9"
      "337d9b8ed29b4505c8e57a06a9a008ecb89ece3a6e809af64342be4367e06ba1be"
      "c131c8944465ba1f5cead836e84932097aea1f6aefc97e84f76219b9dec8afd7a1"
      "d0fa90802bd84b1d021112daf026c60ad958db4247e56dc39d0203010001"_unhex);
}

/**
 * @given a private Ed25519 key generated in golang
 * @when public key derived, test blob signed and signature gets verified
 * @then all the outcomes are the same as in golang
 */
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

/**
 * @given a private RSA2048 key generated in golang
 * @when public key derived, test blob signed and signature gets verified
 * @then all the outcomes are the same as in golang
 */
TEST_F(KeyGoCompatibility, RSA) {
  PrivateKey private_key{
      {Key::Type::RSA,
       "308204a30201000282010100d29170cf0ebe339eb4b4ec2a7642246f8f1566af0df6e1c"
       "9de79219b31ac97dca32ef9fa265fb46c7ad91098eec3cfc1e31df407ed6a5a0a2f1979"
       "01dbf232ad8d5e57dd47fe29ed423b3a8415fac698e982b8e0a32ac857e44591bbdef51"
       "26a9a621efe7267b1e5f6db02f86c217c9266c8faed612723e593a0364650f5e4103053"
       "63f4f08209c912edaea15173277b68ab6d057282143f9b66dd13d518265d0642f494e03"
       "16a52bac226b2783a7b10905a6e793a14d9c9d065b3847d7eab44f5c4c3f838468b020f"
       "6f2fe55fa07ade60b5007ca398ba33910954a901bde2b34932c7c759681d6bbef3e692c"
       "0397200edf21585be7224e3a3daaf55b6879f7f02030100010282010038c444e94d4c31"
       "96639932e1efa7bd32e61c1ae6ae99141ddc0043f316dd34c3a2aa9371c0cea4516a7d0"
       "260785e09b0778e27afcb6d9480925a07a95ca65acb37056c2297ba098b91036eaf154d"
       "da24772f6ab004cd0fddc2088f555ab22f18d62e05b50b1ab17711a9d18f2f7787a1e05"
       "be66a007b10ce5f921d8faf5bdaf394bbb68b5c582edac264cc3eeef893191b6e4e2cb0"
       "60d87b3f6efa90423ce513c8fae23c7d4938378ab9488512f7b340a9aedd1f6d236c01a"
       "df16bf8de79c3255d9a70e6aa051ddcb248b9e79ec4b9cb3f16e63670009cdc6df31912"
       "c6644c9ef492e79ab3298a15ab4cfa68525ade5cb82742c6fe7bb0482c9dd570f8e8002"
       "102818100fbb1c859459a9705dcd6b5600053ad646dfff28edb87cf53ccddff39b522b2"
       "e21d4dc44f1f4d1726bda52ba83246117d42577bff46a2a2af66c4cea6ff3f215864649"
       "17b6895b552d6859771577328c250ed430593447405386e5b263dd882bcd3a7396c5631"
       "21ed1718cfdd2f138faf66fda35643973b65a9f22e4d9146e29302818100d62b8edeea3"
       "3627de02504e8983e92008348fc1257f484d8e9ea9cd597aede49625743c16274279393"
       "277b61f891da11007880c9ab12cccaddbb2ddbe4904efe599f82479049bab5309a989f4"
       "23eee89d232a74e498c220ee4681a0e55e7c3f36d93f517739776d731fd337ed7fbf859"
       "32f96350c994be3314e33336302286e50281803772f5366595270c4d98a7a09cb1d2933"
       "b8095894f67de0f12251e23327a907a2c0683e70278534f5f9c51bfde437d7ae0f0b10a"
       "8e1f2a440619f984e4da9d980195fe4ef7bd7392ea5bc7ff5a4aae82109e1493d7dbcec"
       "b8fa110479e7c62610327e608adfc6902f881a8d98b937da620c464058f22741d73913d"
       "0e2da1362d0281804ce4f2b4e24d74ad510eb9867132f5e4ad3e4512a8f5a7c4e1a7548"
       "bf39bdb3f69c97f102db31a8a87a90349979f7635e87f5b6e0cb801434cfce9682bd60c"
       "2692330ce978ca9ff871ecefa32e7bbdd549dcd9e8e7cb22674a667e046b9f7ce127949"
       "9c8c3bdbbf363854f39d97e241a928cabb5d3ca4dc7b556258aed19586902818100af71"
       "a0b0a23c0f0a79549291b705b7a51234d73b54db0339ff727b8669af76a3be1030b40ae"
       "4ffd8ebc593913d7d80b3e16a67c2433bd627f2d47adb3f3cbaf2326f119f8986384c11"
       "390bc89da38f275c62659f799d21063833caeaed03b10a8433ab6cb6705f854d026959f"
       "69b2248488fcf5287ab86715dbc0974325756f2"_unhex}};

  auto derived = crypto_provider_->derivePublicKey(private_key).value();
  EXPECT_EQ(
      derived.data,
      "30820122300d06092a864886f70d01010105000382010f003082010a0282010100"
      "d29170cf0ebe339eb4b4ec2a7642246f8f1566af0df6e1c9de79219b31ac97dca3"
      "2ef9fa265fb46c7ad91098eec3cfc1e31df407ed6a5a0a2f197901dbf232ad8d5e"
      "57dd47fe29ed423b3a8415fac698e982b8e0a32ac857e44591bbdef5126a9a621e"
      "fe7267b1e5f6db02f86c217c9266c8faed612723e593a0364650f5e410305363f4"
      "f08209c912edaea15173277b68ab6d057282143f9b66dd13d518265d0642f494e0"
      "316a52bac226b2783a7b10905a6e793a14d9c9d065b3847d7eab44f5c4c3f83846"
      "8b020f6f2fe55fa07ade60b5007ca398ba33910954a901bde2b34932c7c759681d"
      "6bbef3e692c0397200edf21585be7224e3a3daaf55b6879f7f0203010001"_unhex);

  auto message{"think of the rapture!"_v};
  const size_t message_len{21};  // here we do not count terminating null char
  ASSERT_EQ(message.size(), message_len);

  auto msg_span = gsl::make_span(message.data(), message_len);
  auto signature = crypto_provider_->sign(msg_span, private_key).value();
  EXPECT_EQ(signature,
            "23127a1173417488c13366c5af09a66699eae8c36a8ce6d2a355b9cadaf35cf02a"
            "5f040c8e5abb2a03d99306060557f2d160b6cc5ba0af72013aae91afc1d7b26a57"
            "2ca25c46e8b80c71a8ba797acca66d339c2dd99ef77fba9d67b475973c016260b5"
            "6b50ec78b2e1cb584ca6c86a9917564c7452330bc8ff4bbe9444d4fb77f5607220"
            "3ae51d8e4bff3d561d3878f2adedeb91e5c7c6bf63e3ccca0250a9729c5cea64ae"
            "34bc9f23fcdeae0dde9025558f5eec52f7c28605dc570e8ffe123642255cbb6cff"
            "a966984a1b403976947e08a914f3a243c0c2bbba07c703ea444caf81dff8f22fd5"
            "77ee81f40e0697066d1f80ff41428a83f0c5b5b6045ce13dc6"_unhex);

  auto verify_result =
      crypto_provider_->verify(msg_span, signature, derived).value();
  ASSERT_TRUE(verify_result);
}

/**
 * @given a private ECDSA key generated in golang
 * @when public key derived, test blob signed and signature gets verified
 * @then all the outcomes are the same as in golang
 */
TEST_F(KeyGoCompatibility, ECDSA) {
  PrivateKey private_key{
      {Key::Type::ECDSA,
       "307702010104209466e35f6cbe89c1b96ef0a58b4bb66913f767581f5b8f669ce50e561"
       "bdd7754a00a06082a8648ce3d030107a144034200047b0c9099a22405b7d425aa607dad"
       "2782b82fa172b31348b7c59aca51fb2c101986d70d59b177a33bbb9b79c1e780db23d1b"
       "7d345a7473d77b9b75b4deaa21997"_unhex}};

  auto derived = crypto_provider_->derivePublicKey(private_key).value();
  EXPECT_EQ(derived.data,
            "3059301306072a8648ce3d020106082a8648ce3d030107034200047b0c9099a224"
            "05b7d425aa607dad2782b82fa172b31348b7c59aca51fb2c101986d70d59b177a3"
            "3bbb9b79c1e780db23d1b7d345a7473d77b9b75b4deaa21997"_unhex);

  auto message{"think of the rapture!"_v};
  const size_t message_len{21};  // here we do not count terminating null char
  ASSERT_EQ(message.size(), message_len);

  auto msg_span = gsl::make_span(message.data(), message_len);
  auto signature = crypto_provider_->sign(msg_span, private_key).value();

  auto verify_own_result =
      crypto_provider_->verify(msg_span, signature, derived);
  ASSERT_TRUE(verify_own_result.has_value());
  ASSERT_TRUE(verify_own_result.value());

  auto go_signature =
      "304502201e045bf3d5e36c7870307ddf7f61577a641054bf21b67c1a233c4e03998d0501"
      "022100f3a41d42dc365a698fa2257181ec6554bbb833ff4dd5a52119558c0aa4a4a0da"_unhex;
  auto verify_go_result =
      crypto_provider_->verify(msg_span, go_signature, derived);
  ASSERT_TRUE(verify_go_result.has_value());
  ASSERT_TRUE(verify_go_result.value());
}

/**
 * @given a private Secp256k1 key generated in golang
 * @when public key derived, test blob signed and signature gets verified
 * @then all the outcomes are the same as in golang
 */
TEST_F(KeyGoCompatibility, Secp256k1) {
  PrivateKey private_key{
      {Key::Type::Secp256k1,
       "7a719128d60097eb45859be6e76a59fc81afe805bf187d354187d2ab45310b6a"_unhex}};

  auto derived = crypto_provider_->derivePublicKey(private_key).value();
  EXPECT_EQ(
      derived.data,
      "02bf00d2b556f8d5fc87b82465c653241ae21420635b374c7c76add17571813dd7"_unhex);

  auto message{"think of the rapture!"_v};
  const size_t message_len{21};  // here we do not count terminating null char
  ASSERT_EQ(message.size(), message_len);

  auto msg_span = gsl::make_span(message.data(), message_len);
  auto signature = crypto_provider_->sign(msg_span, private_key).value();

  auto verify_own_result =
      crypto_provider_->verify(msg_span, signature, derived);
  ASSERT_TRUE(verify_own_result.has_value());
  ASSERT_TRUE(verify_own_result.value());

  auto go_signature =
      "3045022100a6ffadf76999d30c964a40677788f13c89478550d2013e780fe17c265578cd"
      "a90220265a6f8162900c1841913f260dc932f3d61db7b08f11cd356289c7aea71f4d12"_unhex;
  auto verify_go_result =
      crypto_provider_->verify(msg_span, go_signature, derived);
  ASSERT_TRUE(verify_go_result.has_value());
  ASSERT_TRUE(verify_go_result.value());
}
