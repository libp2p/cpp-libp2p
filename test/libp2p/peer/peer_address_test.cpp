/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/peer_address.hpp>

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/multi/multibase_codec/multibase_codec_impl.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <qtils/test/outcome.hpp>

using libp2p::Bytes;
using namespace libp2p::common;
using namespace libp2p::multi;
using namespace libp2p::peer;
using namespace libp2p::common;
using libp2p::common::operator""_unhex;

class PeerAddressTest : public ::testing::Test {
 public:
  std::shared_ptr<MultibaseCodec> codec_ =
      std::make_shared<MultibaseCodecImpl>();

  Bytes hash =
      "af85e416fa66390b3c834cb6b7aeafb8b4b484e7245fd9a9d81e7f3f5f95714f"_unhex;

  const Multihash kDefaultMultihash =
      Multihash::create(HashType::sha256, hash).value();

  const PeerId kDefaultPeerId = PeerId::fromHash(kDefaultMultihash).value();

  const std::string kEncodedDefaultPeerId = kDefaultPeerId.toBase58();

  const Multiaddress kDefaultAddress =
      Multiaddress::create("/ip4/192.168.0.1/tcp/228").value();

  const std::string kAddressString =
      std::string{kDefaultAddress.getStringAddress()} + "/p2p/"
      + kEncodedDefaultPeerId;
};

/**
 * @given well-formed peer address string
 * @when creating a Peeraddress from it
 * @then creation is successful
 */
TEST_F(PeerAddressTest, FromStringSuccess) {
  ASSERT_OUTCOME_SUCCESS(address, PeerAddress::create(kAddressString));
  EXPECT_EQ(address.toString(), kAddressString);
  EXPECT_EQ(address.getId(), kDefaultPeerId);
  EXPECT_EQ(address.getAddress(), kDefaultAddress);
}

/**
 * @given peer address string without peer's id
 * @when creating a Peeraddress from it
 * @then creation fails
 */
TEST_F(PeerAddressTest, FromStringNoId) {
  EXPECT_FALSE(PeerAddress::create(kDefaultAddress.getStringAddress()));
}

/**
 * @given peer address string with an ill-formed multiaddress
 * @when creating a Peeraddress from it
 * @then creation fails
 */
TEST_F(PeerAddressTest, FromStringIllFormedAddress) {
  EXPECT_FALSE(PeerAddress::create("/192.168.0.1/ipfs/something"));
}

/**
 * @given peer address string with id, which is not base58-encoded
 * @when creating a Peeraddress from it
 * @then creation fails
 */
TEST_F(PeerAddressTest, FromStringIdNotB58) {
  EXPECT_FALSE(PeerAddress::create(
      std::string{kDefaultAddress.getStringAddress()} + "/ipfs/something"));
}

/**
 * @given peer address string with base58-encoded id, which is not sha256
 * multihash
 * @when creating a Peeraddress from it
 * @then creation is successful
 */
TEST_F(PeerAddressTest, FromStringIdNotSha256) {
  auto some_str_b58 =
      codec_->encode(Bytes{0x11, 0x22}, MultibaseCodec::Encoding::BASE58);
  EXPECT_FALSE(
      PeerAddress::create(std::string{kDefaultAddress.getStringAddress()}
                          + "/ipfs/" + some_str_b58));
}

/**
 * @given well-formed peer info structure
 * @when creating a Peeraddress from it
 * @then creation is successful
 */
TEST_F(PeerAddressTest, FromInfoSuccess) {
  ASSERT_OUTCOME_SUCCESS(
      address,
      PeerAddress::create(PeerInfo{kDefaultPeerId, {kDefaultAddress}}));
  EXPECT_EQ(address.toString(), kAddressString);
  EXPECT_EQ(address.getId(), kDefaultPeerId);
  EXPECT_EQ(address.getAddress(), kDefaultAddress);
}

/**
 * @given peer info structure without any multiaddresses
 * @when creating a Peeraddress from it
 * @then creation is successful
 */
TEST_F(PeerAddressTest, FromInfoNoAddresses) {
  EXPECT_FALSE(PeerAddress::create(PeerInfo{kDefaultPeerId, {}}));
}

/**
 * @given PeerId @and Multiaddress structures
 * @when creating a Peeraddress from them
 * @then creation is successful
 */
TEST_F(PeerAddressTest, FromDistinctSuccess) {
  ASSERT_OUTCOME_SUCCESS(address,
                         PeerAddress::create(kDefaultPeerId, kDefaultAddress));
  EXPECT_EQ(address.toString(), kAddressString);
  EXPECT_EQ(address.getId(), kDefaultPeerId);
  EXPECT_EQ(address.getAddress(), kDefaultAddress);
}
