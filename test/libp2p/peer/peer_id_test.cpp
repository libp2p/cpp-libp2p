/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id.hpp"

#include <gtest/gtest.h>
#include <libp2p/crypto/key_generator/key_generator_impl.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>
#include <libp2p/crypto/key_validator/key_validator_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include <testutil/literals.hpp>
#include <testutil/outcome.hpp>

using namespace libp2p::crypto;
using namespace libp2p::peer;
using namespace libp2p::multi;
using namespace libp2p::multi::detail;

class PeerIdTest : public ::testing::Test {
 public:
  const Buffer kBuffer{0x11, 0x22, 0x33};
};

/**
 * @given public key
 * @when creating a PeerId from it
 * @then creation is successful
 */
TEST_F(PeerIdTest, FromPubkeySuccess) {
  PublicKey pubkey{};
  pubkey.type = Key::Type::RSA;
  pubkey.data = kBuffer;

  auto hash = libp2p::crypto::sha256(pubkey.data);
  EXPECT_OUTCOME_TRUE(multihash,
                      Multihash::create(libp2p::multi::sha256,
                                        Buffer{hash.begin(), hash.end()}))

  EXPECT_OUTCOME_TRUE(peer_id, PeerId::fromPublicKey(ProtobufKey{pubkey.data}))
  EXPECT_EQ(peer_id.toBase58(), encodeBase58(multihash.toBuffer()));
  EXPECT_EQ(peer_id.toMultihash(), multihash);
}

/**
 * @given base58-encoded sha256 multihash
 * @when creating a PeerId from it
 * @then creation is successful
 */
TEST_F(PeerIdTest, FromBase58Success) {
  EXPECT_OUTCOME_TRUE(hash, Multihash::create(libp2p::multi::sha256, kBuffer));
  auto hash_b58 = encodeBase58(hash.toBuffer());

  EXPECT_OUTCOME_TRUE(peer_id, PeerId::fromBase58(hash_b58))
  EXPECT_EQ(peer_id.toBase58(), hash_b58);
  EXPECT_EQ(peer_id.toMultihash(), hash);
}

/**
 * @given some random string
 * @when creating a PeerId from it
 * @then creation fails
 */
TEST_F(PeerIdTest, FromBase58NotBase58) {
  EXPECT_FALSE(PeerId::fromBase58("some random string"));
}

/**
 * @given base58-encoded string, which is not a multihash
 * @when creating a PeerId from it
 * @then creation fails
 */
TEST_F(PeerIdTest, FromBase58IcorrectHash) {
  auto not_hash_b58 = encodeBase58(kBuffer);

  EXPECT_FALSE(PeerId::fromBase58(not_hash_b58));
}

/**
 * @given base58-encoded non-sha256 multihash
 * @when creating a PeerId from it
 * @then creation fails
 */
TEST_F(PeerIdTest, FromBase58NotSha256) {
  EXPECT_OUTCOME_TRUE(hash, Multihash::create(libp2p::multi::sha512, kBuffer))
  auto hash_b58 = encodeBase58(hash.toBuffer());

  EXPECT_FALSE(PeerId::fromBase58(hash_b58));
}

/**
 * @given sha256 multihash
 * @when creating a PeerId from it
 * @then creation is successful
 */
TEST_F(PeerIdTest, FromHashSuccess) {
  EXPECT_OUTCOME_TRUE(hash, Multihash::create(libp2p::multi::sha256, kBuffer));
  auto hash_b58 = encodeBase58(hash.toBuffer());

  EXPECT_OUTCOME_TRUE(peer_id, PeerId::fromHash(hash))
  EXPECT_EQ(peer_id.toBase58(), hash_b58);
  EXPECT_EQ(peer_id.toMultihash(), hash);
}

/**
 * @given non-sha256 multihash
 * @when creating a PeerId from it
 * @then creation fails
 */
TEST_F(PeerIdTest, FromHashNotSha256) {
  EXPECT_OUTCOME_TRUE(hash, Multihash::create(libp2p::multi::sha512, kBuffer))

  EXPECT_FALSE(PeerId::fromHash(hash));
}

TEST_F(PeerIdTest, GoCompat) {
  PublicKey pk;
  pk.type = Key::Type::Ed25519;
  pk.data = std::vector<unsigned char>{113, 102, 217, 191, 10,  90,  24,  250,
                                       61,  38,  253, 236, 80,  96,  208, 191,
                                       54,  191, 159, 137, 178, 24,  1,   71,
                                       241, 174, 51,  55,  158, 172, 60,  71};
  auto csprng = std::make_shared<random::BoostRandomGenerator>();
  auto generator = std::make_shared<KeyGeneratorImpl>(*csprng);
  auto validator = std::make_shared<validator::KeyValidatorImpl>(generator);
  marshaller::KeyMarshallerImpl marshaller(validator);
  pk.data = marshaller.marshal(pk).value().key;
  EXPECT_OUTCOME_TRUE(id, PeerId::fromPublicKey(ProtobufKey{pk.data}))
  ASSERT_EQ(id.toBase58(), "QmSv2EN7qEHxVEu6RNeVSQGK8RqjRuc3RFfEJLHMVJmt1K");
}
