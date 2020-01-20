/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <algorithm>

#include <gtest/gtest.h>
#include "libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp"
#include "testutil/outcome.hpp"

using libp2p::crypto::secp256k1::PrivateKey;
using libp2p::crypto::secp256k1::PublicKey;
using libp2p::crypto::secp256k1::Secp256k1ProviderImpl;
using libp2p::crypto::secp256k1::Signature;

/**
 * @brief Pre-generated key pair and signature for sample message
 * @using reference implementation from github.com/libp2p/go-libp2p-core
 */

#define SAMPLE_PRIVATE_KEY_BYTES                                             \
  0xD9, 0x90, 0xE0, 0xF2, 0x4F, 0xFC, 0x86, 0x8C, 0xD6, 0xAC, 0x4D, 0xBA,    \
      0xE1, 0xB3, 0x30, 0x82, 0x31, 0x0, 0xE7, 0x26, 0x75, 0x38, 0x95, 0xC1, \
      0x18, 0x4B, 0x6E, 0xC3, 0x88, 0x50, 0x64, 0xD1

#define SAMPLE_PUBLIC_KEY_BYTES                                               \
  0x3, 0x1E, 0x24, 0x4C, 0xB9, 0x88, 0xD1, 0xB8, 0x0, 0x8C, 0xAD, 0x7A, 0xB8, \
      0x63, 0x6F, 0xEC, 0xC5, 0xA1, 0x1A, 0xE9, 0xC3, 0x4A, 0x5C, 0xF, 0xEB,  \
      0x2F, 0xBB, 0xC7, 0x56, 0xF2, 0xD6, 0xB0, 0x2C

#define SAMPLE_SIGNATURE_BYTES                                                 \
  0x30, 0x44, 0x2, 0x20, 0x7A, 0x89, 0xB5, 0x9B, 0x1F, 0x78, 0x6D, 0x20, 0x3B, \
      0xF1, 0x8F, 0x94, 0x77, 0x34, 0xB9, 0x7A, 0x53, 0xD, 0x5C, 0x41, 0x81,   \
      0x43, 0x19, 0x8C, 0xD3, 0x1C, 0x3B, 0xC6, 0xC6, 0xB6, 0x9F, 0x65, 0x2,   \
      0x20, 0x50, 0xD2, 0x25, 0xC6, 0x47, 0xF7, 0x34, 0x59, 0x4A, 0x92, 0x66,  \
      0x5A, 0x31, 0xC6, 0xD5, 0xC8, 0xC5, 0xA8, 0x88, 0xCC, 0x3D, 0x4B, 0x8F,  \
      0x1A, 0x65, 0x35, 0x53, 0xE6, 0x3A, 0x25, 0x3C, 0xF2

#define SAMPLE_MESSAGE_BYTES                                                  \
  0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x21, 0x20, 0x61, 0x6E, 0x64, 0x20, 0x77,     \
      0x65, 0x6C, 0x63, 0x6F, 0x6D, 0x65, 0x20, 0x74, 0x6F, 0x20, 0x73, 0x6F, \
      0x6D, 0x65, 0x20, 0x61, 0x77, 0x65, 0x73, 0x6F, 0x6D, 0x65, 0x20, 0x63, \
      0x72, 0x79, 0x70, 0x74, 0x6F, 0x20, 0x70, 0x72, 0x69, 0x6D, 0x69, 0x74, \
      0x69, 0x76, 0x65, 0x73

class Secp256k1ProviderTest : public ::testing::Test {
 protected:
  PublicKey sample_public_key_{SAMPLE_PUBLIC_KEY_BYTES};
  PrivateKey sample_private_key_{SAMPLE_PRIVATE_KEY_BYTES};
  Signature sample_signature_{SAMPLE_SIGNATURE_BYTES};
  std::vector<uint8_t> data_{SAMPLE_MESSAGE_BYTES};
  gsl::span<uint8_t> message_{data_.data(), static_cast<int32_t>(data_.size())};
  Secp256k1ProviderImpl provider_;
};

/**
 * @given Pre-generated secp256k1 private and public keys
 * @when Deriving public key from private
 * @then Derived public key must be the same as pre-generated
 */
TEST_F(Secp256k1ProviderTest, PublicKeyDerivationSuccess) {
  Secp256k1ProviderImpl provider;
  EXPECT_OUTCOME_TRUE(derivedPublicKey, provider.derive(sample_private_key_));
  ASSERT_EQ(derivedPublicKey, sample_public_key_);
};

/**
 * @given Pre-generated secp256k1 key pair, sample message and signature
 * @when Verifying pre-generated signature
 * @then Verification of the pre-generated signature must be successful
 */
TEST_F(Secp256k1ProviderTest, PreGeneratedSignatureVerificationSuccess) {
  EXPECT_OUTCOME_TRUE(
      verificationResult,
      provider_.verify(message_, sample_signature_, sample_public_key_));
  ASSERT_TRUE(verificationResult);
}

/**
 * @given Sample message to sign and verify
 * @when Generating new key pair, signature and verification of this signature
 * @then Generating key pair, signature and it's verification must be successful
 */
TEST_F(Secp256k1ProviderTest, GenerateSignatureSuccess) {
  EXPECT_OUTCOME_TRUE(keyPair, provider_.generate());
  EXPECT_OUTCOME_TRUE(signature, provider_.sign(message_, keyPair.private_key));
  EXPECT_OUTCOME_TRUE(
      verificationResult,
      provider_.verify(message_, signature, keyPair.public_key));
  ASSERT_TRUE(verificationResult);
}

/**
 * @given Sample message to sign and verify
 * @when Generating new signature and verifying with different public key
 * @then Signature for different public key must be invalid
 */
TEST_F(Secp256k1ProviderTest, VerifySignatureInvalidKeyFailure) {
  EXPECT_OUTCOME_TRUE(firstKeyPair, provider_.generate());
  EXPECT_OUTCOME_TRUE(secondKeyPair, provider_.generate());
  EXPECT_OUTCOME_TRUE(signature,
                      provider_.sign(message_, firstKeyPair.private_key));
  EXPECT_OUTCOME_TRUE(
      verificationResult,
      provider_.verify(message_, signature, secondKeyPair.public_key));
  ASSERT_FALSE(verificationResult);
}

/**
 * @given Key pair and sample message to sign
 * @when Generating and verifying invalid signature
 * @then Invalid signature verification must be unsuccessful
 */
TEST_F(Secp256k1ProviderTest, VerifyInvalidSignaturFailure) {
  EXPECT_OUTCOME_TRUE(signature, provider_.sign(message_, sample_private_key_));
  message_[0] = 0;  // Modify sample message
  EXPECT_OUTCOME_TRUE(
      verificationResult,
      provider_.verify(message_, signature, sample_public_key_));
  ASSERT_FALSE(verificationResult);
}
