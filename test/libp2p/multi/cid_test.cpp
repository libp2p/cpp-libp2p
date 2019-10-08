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
#include <libp2p/multi/uvarint.hpp>
#include <testutil/outcome.hpp>
#include <testutil/printers.hpp>

using libp2p::multi::ContentIdentifier;
using libp2p::multi::ContentIdentifierCodec;
using libp2p::multi::HashType;
using libp2p::multi::MultibaseCodec;
using libp2p::multi::MultibaseCodecImpl;
using libp2p::multi::Multihash;
using libp2p::multi::UVarint;
using libp2p::common::operator""_multihash;
using libp2p::common::operator""_v;

class CidEncodeTest
    : public testing::TestWithParam<
          std::pair<ContentIdentifier, outcome::result<std::vector<uint8_t>>>> {
};

TEST(CidTest, Create) {
  ContentIdentifier c(ContentIdentifier::Version::V0, UVarint(32),
                      "123456"_multihash);
  ASSERT_EQ(c.content_address, "123456"_multihash);
}

TEST_P(CidEncodeTest, Encode) {
  auto [cid, expectation] = GetParam();
  auto bytes = ContentIdentifierCodec::encode(cid);
  if (expectation) {
    ASSERT_TRUE(std::equal(bytes.value().begin(), bytes.value().end(),
                           expectation.value().begin()))
        << libp2p::common::hex_lower(bytes.value());
  } else {
    ASSERT_EQ(bytes.error(), expectation.error()) << bytes.error().message();
  }
}

class CidDecodeTest
    : public testing::TestWithParam<
          std::pair<std::string, outcome::result<ContentIdentifier>>> {
 public:
  void SetUp() {
    base_codec = std::make_shared<MultibaseCodecImpl>();
  }

  std::shared_ptr<MultibaseCodec> base_codec;
};

TEST_P(CidDecodeTest, BaseDecode) {
  auto [base_cid, expectation] = GetParam();
  EXPECT_OUTCOME_TRUE(cid_bytes, base_codec->decode(base_cid));
  auto cid = ContentIdentifierCodec::decode(cid_bytes);
  if (expectation) {
    ASSERT_EQ(cid.value(), expectation.value());
  } else {
    ASSERT_EQ(cid.error(), expectation.error()) << cid.error().message();
  }
}

const std::vector<
    std::pair<ContentIdentifier, outcome::result<std::vector<uint8_t>>>>
    encodeSuite{
        {ContentIdentifier(
             ContentIdentifier::Version::V0, UVarint(42),
             "12200000000000000000000000000000000000000000000000000000000000000000"_multihash),
         ContentIdentifierCodec::EncodeError::INVALID_CONTENT_TYPE},
        {ContentIdentifier(
             ContentIdentifier::Version::V0, UVarint(0x70),
             "12200000000000000000000000000000000000000000000000000000000000000000"_multihash),
         "12200000000000000000000000000000000000000000000000000000000000000000"_v}};

INSTANTIATE_TEST_CASE_P(EncodeTests, CidEncodeTest,
                        testing::ValuesIn(encodeSuite));

const std::vector<
    std::pair<std::string, outcome::result<ContentIdentifier>>>
    decodeSuite{
        {"zb2rhe5P4gXftAwvA4eXQ5HJwsER2owDyS9sKaQRRVQPn93bA",
         ContentIdentifier(
             ContentIdentifier::Version::V1, UVarint(0x55),
             "12206e6ff7950a36187a801613426e858dce686cd7d7e3c0fc42ee0330072d245c95"_multihash)}};

INSTANTIATE_TEST_CASE_P(DecodeTests, CidDecodeTest,
                        testing::ValuesIn(decodeSuite));
