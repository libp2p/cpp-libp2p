/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/common/literals.hpp>
#include <libp2p/multi/content_identifier.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/multibase_codec/multibase_codec_impl.hpp>
#include <libp2p/multi/multicodec_type.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <testutil/outcome.hpp>

using libp2p::multi::ContentIdentifier;
using libp2p::multi::ContentIdentifierCodec;
using libp2p::multi::HashType;
using libp2p::multi::MultibaseCodec;
using libp2p::multi::MultibaseCodecImpl;
using libp2p::multi::MulticodecType;
using libp2p::multi::Multihash;
using libp2p::multi::UVarint;
using libp2p::common::operator""_multihash;
using libp2p::common::operator""_unhex;

const Multihash ZERO_MULTIHASH =
    "12200000000000000000000000000000000000000000000000000000000000000000"_multihash;
const Multihash EXAMPLE_MULTIHASH =
    "12206e6ff7950a36187a801613426e858dce686cd7d7e3c0fc42ee0330072d245c95"_multihash;

TEST(CidTest, PrettyString) {
  ContentIdentifier c1(ContentIdentifier::Version::V1, MulticodecType::IDENTITY,
                       ZERO_MULTIHASH);
  ASSERT_EQ(c1.toPrettyString("base58"),
            "base58 - cidv1 - identity - sha2-256-256-"
                + libp2p::common::hex_lower(ZERO_MULTIHASH.getHash()));
  ContentIdentifier c2(ContentIdentifier::Version::V0, MulticodecType::DAG_PB,
                       EXAMPLE_MULTIHASH);
  ASSERT_EQ(c2.toPrettyString("base64"),
            "base64 - cidv0 - dag-pb - sha2-256-256-"
                + libp2p::common::hex_lower(EXAMPLE_MULTIHASH.getHash()));
}

/**
 * @given CID of different versions
 * @when compare CIDs
 * @then lesser version is always less
 */
TEST(CidTest, CompareDifferentVersion) {
  ContentIdentifier c0_v0(ContentIdentifier::Version::V0,
                          MulticodecType::IDENTITY, ZERO_MULTIHASH);
  ContentIdentifier c0_v1(ContentIdentifier::Version::V1,
                          MulticodecType::IDENTITY, ZERO_MULTIHASH);
  ASSERT_TRUE(c0_v0 < c0_v1);
  ASSERT_FALSE(c0_v0 < c0_v0);
  ASSERT_FALSE(c0_v1 < c0_v1);

  ContentIdentifier c1_v1(ContentIdentifier::Version::V1,
                          MulticodecType::IDENTITY, ZERO_MULTIHASH);
  ASSERT_TRUE(c0_v0 < c1_v1);

  ContentIdentifier c2_v0(ContentIdentifier::Version::V0, MulticodecType::SHA1,
                          ZERO_MULTIHASH);
  ASSERT_TRUE(c0_v0 < c2_v0);
  ASSERT_TRUE(c0_v0 < c0_v1);
}

/**
 * @given CID of different types
 * @when compare CIDs
 * @then lesser type is always less
 */
TEST(CidTest, CompareDifferentTypes) {
  ContentIdentifier c1(ContentIdentifier::Version::V1, MulticodecType::IDENTITY,
                       ZERO_MULTIHASH);
  ContentIdentifier c2(ContentIdentifier::Version::V1, MulticodecType::SHA1,
                       ZERO_MULTIHASH);
  ASSERT_TRUE(c1 < c2);
  ASSERT_FALSE(c2 < c1);
  ASSERT_FALSE(c1 < c1);
  ASSERT_FALSE(c2 < c2);
}

/**
 * @given CID of different hashes
 * @when compare CIDs
 * @then lesser hash is always less
 */
TEST(CidTest, CompareDifferentHashes) {
  ContentIdentifier c1(ContentIdentifier::Version::V1, MulticodecType::IDENTITY,
                       ZERO_MULTIHASH);
  ContentIdentifier c2(ContentIdentifier::Version::V1, MulticodecType::IDENTITY,
                       EXAMPLE_MULTIHASH);
  ASSERT_TRUE(c1 < c2);
  ASSERT_FALSE(c2 < c1);
  ASSERT_FALSE(c1 < c1);
  ASSERT_FALSE(c2 < c2);
}

class CidEncodeTest
    : public testing::TestWithParam<std::pair<
          ContentIdentifier, libp2p::outcome::result<std::vector<uint8_t>>>> {};

TEST(CidTest, Create) {
  ContentIdentifier c(ContentIdentifier::Version::V0, MulticodecType::IDENTITY,
                      EXAMPLE_MULTIHASH);
  ASSERT_EQ(c.content_address, EXAMPLE_MULTIHASH);
}

TEST_P(CidEncodeTest, Encode) {
  auto [cid, expectation] = GetParam();
  auto bytes = ContentIdentifierCodec::encode(cid);
  if (expectation) {
    auto bytes_value = bytes.value();
    auto expectation_value = expectation.value();
    ASSERT_TRUE(std::equal(bytes_value.begin(), bytes_value.end(),
                           expectation_value.begin()))
        << libp2p::common::hex_lower(bytes_value);
  } else {
    ASSERT_EQ(bytes.error(), expectation.error()) << bytes.error().message();
  }
}

class CidDecodeTest
    : public testing::TestWithParam<std::pair<
          std::vector<uint8_t>, libp2p::outcome::result<ContentIdentifier>>> {
 public:
  void SetUp() {
    base_codec = std::make_shared<MultibaseCodecImpl>();
  }

  std::shared_ptr<MultibaseCodec> base_codec;
};

TEST_P(CidDecodeTest, Decode) {
  auto [cid_bytes, expectation] = GetParam();
  auto cid = ContentIdentifierCodec::decode(cid_bytes);
  if (expectation) {
    ASSERT_EQ(cid.value(), expectation.value());
  } else {
    ASSERT_EQ(cid.error(), expectation.error()) << cid.error().message();
  }
}

class CidEncodeDecodeTest : public testing::TestWithParam<ContentIdentifier> {};

TEST_P(CidEncodeDecodeTest, DecodedMatchesOriginal) {
  auto cid = GetParam();
  EXPECT_OUTCOME_TRUE(bytes, ContentIdentifierCodec::encode(cid));
  EXPECT_OUTCOME_TRUE(dec_cid, ContentIdentifierCodec::decode(bytes));
  ASSERT_EQ(cid, dec_cid);
}

const std::vector<
    std::pair<ContentIdentifier, libp2p::outcome::result<std::vector<uint8_t>>>>
    encodeSuite{{ContentIdentifier(ContentIdentifier::Version::V0,
                                   MulticodecType::SHA1, ZERO_MULTIHASH),
                 ContentIdentifierCodec::EncodeError::INVALID_CONTENT_TYPE},
                {ContentIdentifier(ContentIdentifier::Version::V0,
                                   MulticodecType::DAG_PB, ZERO_MULTIHASH),
                 ZERO_MULTIHASH.toBuffer()}};

INSTANTIATE_TEST_CASE_P(EncodeTests, CidEncodeTest,
                        testing::ValuesIn(encodeSuite));

const std::vector<
    std::pair<std::vector<uint8_t>, libp2p::outcome::result<ContentIdentifier>>>
    decodeSuite{{EXAMPLE_MULTIHASH.toBuffer(),
                 ContentIdentifier(ContentIdentifier::Version::V0,
                                   MulticodecType::DAG_PB, EXAMPLE_MULTIHASH)}};

INSTANTIATE_TEST_CASE_P(DecodeTests, CidDecodeTest,
                        testing::ValuesIn(decodeSuite));

const std::vector<ContentIdentifier> encodeDecodeSuite = {
    ContentIdentifier(ContentIdentifier::Version::V0, MulticodecType::DAG_PB,
                      EXAMPLE_MULTIHASH),
    ContentIdentifier(ContentIdentifier::Version::V1, MulticodecType::IDENTITY,
                      ZERO_MULTIHASH),
    ContentIdentifier(ContentIdentifier::Version::V1, MulticodecType::SHA1,
                      EXAMPLE_MULTIHASH)};

INSTANTIATE_TEST_CASE_P(EncodeDecodeTest, CidEncodeDecodeTest,
                        testing::ValuesIn(encodeDecodeSuite));
