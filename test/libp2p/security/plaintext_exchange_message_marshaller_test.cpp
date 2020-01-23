/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <generated/security/plaintext/protobuf/plaintext.pb.h>
#include <libp2p/crypto/key.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/security/plaintext/exchange_message_marshaller_impl.hpp>
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "testutil/outcome.hpp"

using libp2p::crypto::Key;
using libp2p::crypto::ProtobufKey;
using libp2p::crypto::PublicKey;
using libp2p::crypto::marshaller::KeyMarshallerMock;
using libp2p::crypto::protobuf::KeyType;
using libp2p::peer::PeerId;
using libp2p::security::plaintext::ExchangeMessage;
using libp2p::security::plaintext::ExchangeMessageMarshaller;
using libp2p::security::plaintext::ExchangeMessageMarshallerImpl;
using testing::_;
using testing::Return;
using ProbufPubKey = libp2p::crypto::protobuf::PublicKey;

class ExchangeMessageMarshallerTest : public testing::Test {
 public:
  void SetUp() {
    key_marshaller = std::make_shared<KeyMarshallerMock>();
    ProbufPubKey pbpk;
    pbpk.set_type(KeyType::Ed25519);
    pbpk.set_data(pk.data.data(), pk.data.size());
    pubkey_bytes.resize(pbpk.ByteSizeLong());
    pbpk.SerializeToArray(pubkey_bytes.data(), pubkey_bytes.size());
    marshaller =
        std::make_shared<ExchangeMessageMarshallerImpl>(key_marshaller);
  }

  std::shared_ptr<KeyMarshallerMock> key_marshaller;
  std::shared_ptr<ExchangeMessageMarshaller> marshaller;
  PublicKey pk{{Key::Type::Ed25519, std::vector<uint8_t>(255, 1)}};
  std::vector<uint8_t> pubkey_bytes;
};

/**
 * @given a peer id and a public key
 * @when serializing an exchange message with them to protobuf and back
 * @then the decoded message matches the original one
 */
TEST_F(ExchangeMessageMarshallerTest, ToProtobufAndBack) {
  EXPECT_CALL(*key_marshaller, marshal(pk))
      .WillOnce(Return(ProtobufKey{pubkey_bytes}));
  EXPECT_CALL(*key_marshaller, unmarshalPublicKey(_)).WillOnce(Return(pk));
  EXPECT_OUTCOME_TRUE(pid, PeerId::fromPublicKey(ProtobufKey{pk.data}))
  ExchangeMessage msg{.pubkey = pk, .peer_id = pid};
  EXPECT_OUTCOME_TRUE(bytes, marshaller->marshal(msg));
  EXPECT_OUTCOME_TRUE(dec_msg, marshaller->unmarshal(bytes));
  ASSERT_EQ(msg.peer_id, dec_msg.first.peer_id);
  ASSERT_EQ(msg.pubkey, dec_msg.first.pubkey);
}

/**
 * @given a peer id and a public key
 * @when serializing an exchange message with them to protobuf and key
 * marshaller gives invalid output
 * @then the message marshaller yields an error
 */
TEST_F(ExchangeMessageMarshallerTest, MarshalError) {
  EXPECT_CALL(*key_marshaller, marshal(pk))
      .WillOnce(Return(ProtobufKey{std::vector<uint8_t>(32, 1)}));
  EXPECT_OUTCOME_TRUE(pid, PeerId::fromPublicKey(ProtobufKey{pk.data}))
  ExchangeMessage msg{.pubkey = pk, .peer_id = pid};
  EXPECT_OUTCOME_FALSE_1(marshaller->marshal(msg));
}

/**
 * @given a peer id and a public key
 * @when deserializing an exchange message with them from protobuf and key
 * marshaller yields an error
 * @then the message marshaller yields an error
 */
TEST_F(ExchangeMessageMarshallerTest, UnmarshalError) {
  EXPECT_CALL(*key_marshaller, marshal(pk))
      .WillOnce(Return(ProtobufKey{pubkey_bytes}));
  EXPECT_CALL(*key_marshaller, unmarshalPublicKey(_))
      .WillOnce(
          Return(libp2p::crypto::CryptoProviderError::FAILED_UNMARSHAL_DATA));
  EXPECT_OUTCOME_TRUE(pid, PeerId::fromPublicKey(ProtobufKey{pk.data}))
  ExchangeMessage msg{.pubkey = pk, .peer_id = pid};
  EXPECT_OUTCOME_TRUE(bytes, marshaller->marshal(msg));
  EXPECT_OUTCOME_FALSE_1(marshaller->unmarshal(bytes));
}
