/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/crypto/common_functions.hpp>
#include <libp2p/crypto/x25519_provider/x25519_provider_impl.hpp>
#include <testutil/outcome.hpp>

using libp2p::crypto::x25519::PrivateKey;
using libp2p::crypto::x25519::PublicKey;
using libp2p::crypto::x25519::X25519ProviderImpl;
using libp2p::common::operator""_unhex;
using libp2p::crypto::asArray;
using libp2p::crypto::asVector;

/**
 * Fixture to test that Diffie Hellman secret derivation via X25519 done right.
 * Reference values are derived from Go impl of the test.
 */
class X25519Fixture : public ::testing::Test {
 public:
  X25519ProviderImpl provider;
  PrivateKey privkey = asArray<PrivateKey>(
      "6d8e72d53e0f8582f52169bf7f6c60ddb7e0fbb83af97a11cff02f1bf21bbf7c"_unhex);
  PublicKey pubkey = asArray<PublicKey>(
      "502d10724db25437888bcd8e3e473ae226cb746740c2bb67fab6a31c650cb228"_unhex);
  std::vector<uint8_t>
      secret =  ///< just a scalar multiplication of privkey and pubkey
      "536b2256eb1e028551b9021cf1c6b850cbd6718794fbf85689397a3b0a53ea6b"_unhex;
};

/**
 * @given a private key as bytes vector
 * @when its public key counterpart is derived
 * @then the public key bytes are equal to expected
 */
TEST_F(X25519Fixture, GoKeyCompatibility) {
  EXPECT_OUTCOME_TRUE(public_key, provider.derive(privkey));
  ASSERT_EQ(public_key, pubkey);
}

/**
 * @given the pair of public and private keys
 * @when DH X25519 shared secret gets derived
 * @then the result equals to the expected
 */
TEST_F(X25519Fixture, GoDiffieHellmanCompatibility) {
  EXPECT_OUTCOME_TRUE(shared_secret, provider.dh(privkey, pubkey));
  ASSERT_EQ(shared_secret, secret);
}

/**
 * @given a predefined keypair and run-time generated keypair
 * @when shared secret is computed for both parties (keypairs)
 * @then the shared secrets are the same
 */
TEST_F(X25519Fixture, SharedSecret) {
  EXPECT_OUTCOME_TRUE(peer, provider.generate());
  EXPECT_OUTCOME_TRUE(secret1, provider.dh(privkey, peer.public_key));
  EXPECT_OUTCOME_TRUE(secret2, provider.dh(peer.private_key, pubkey));
  ASSERT_EQ(secret1, secret2);
}
