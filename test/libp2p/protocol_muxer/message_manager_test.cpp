/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"

#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/protocol_muxer/multiselect/multiselect.hpp"

using kagome::common::Buffer;
using libp2p::multi::Multihash;
using libp2p::multi::UVarint;
using libp2p::peer::Protocol;
using libp2p::protocol_muxer::MessageManager;
using libp2p::protocol_muxer::Multiselect;

using MessageType = MessageManager::MultiselectMessage::MessageType;

class MessageManagerTest : public ::testing::Test {
  static constexpr std::string_view kMultiselectHeaderProtocol =
      "/multistream-select/1.0.0";

 public:
  const std::vector<Protocol> kDefaultProtocols{
      "/plaintext/1.0.0", "/ipfs-dht/0.2.3", "/http/w3id.org/http/1.1"};
  static constexpr uint64_t kProtocolsVarintsSize = 3;
  static constexpr uint64_t kProtocolsListBytesSize = 60;
  static constexpr uint64_t kProtocolsNumber = 3;

  const Buffer kOpeningMsg =
      Buffer{}
          .put(UVarint{kMultiselectHeaderProtocol.size() + 1}.toBytes())
          .put(kMultiselectHeaderProtocol)
          .put("\n");
  const Buffer kLsMsg = Buffer{}.put(UVarint{3}.toBytes()).put("ls\n");
  const Buffer kNaMsg = Buffer{}.put(UVarint{3}.toBytes()).put("na\n");
  const Buffer kProtocolMsg =
      Buffer{}
          .put(UVarint{kDefaultProtocols[0].size() + 1}.toBytes())
          .put(kDefaultProtocols[0])
          .put("\n");
  const Buffer kProtocolsMsg =
      Buffer{}
          .put(UVarint{kProtocolsVarintsSize}.toBytes())
          .put(UVarint{kProtocolsListBytesSize}.toBytes())
          .put(UVarint{kProtocolsNumber}.toBytes())
          .put("\n")
          .put(UVarint{kDefaultProtocols[0].size() + 1}.toBytes())
          .put(kDefaultProtocols[0])
          .put("\n")
          .put(UVarint{kDefaultProtocols[1].size() + 1}.toBytes())
          .put(kDefaultProtocols[1])
          .put("\n")
          .put(UVarint{kDefaultProtocols[2].size() + 1}.toBytes())
          .put(kDefaultProtocols[2])
          .put("\n");
};

/**
 * @given message manager
 * @when getting an opening message from it
 * @then well-formed opening message is returned
 */
TEST_F(MessageManagerTest, ComposeOpeningMessage) {
  auto opening_msg = MessageManager::openingMsg();
  ASSERT_EQ(opening_msg, kOpeningMsg);
}

/**
 * @given message manager
 * @when getting an ls message from it
 * @then well-formed ls message is returned
 */
TEST_F(MessageManagerTest, ComposeLsMessage) {
  auto ls_msg = MessageManager::lsMsg();
  ASSERT_EQ(ls_msg, kLsMsg);
}

/**
 * @given message manager
 * @when getting an na message from it
 * @then well-formed na message is returned
 */
TEST_F(MessageManagerTest, ComposeNaMessage) {
  auto na_msg = MessageManager::naMsg();
  ASSERT_EQ(na_msg, kNaMsg);
}

/**
 * @given message manager @and protocol
 * @when getting a protocol message from it
 * @then well-formed protocol message is returned
 */
TEST_F(MessageManagerTest, ComposeProtocolMessage) {
  auto protocol_msg = MessageManager::protocolMsg(kDefaultProtocols[0]);
  ASSERT_EQ(protocol_msg, kProtocolMsg);
}

/**
 * @given message manager @and protocols
 * @when getting a protocols message from it
 * @then well-formed protocols message is returned
 */
TEST_F(MessageManagerTest, ComposeProtocolsMessage) {
  auto protocols_msg = MessageManager::protocolsMsg(kDefaultProtocols);
  ASSERT_EQ(protocols_msg, kProtocolsMsg);
}

/**
 * @given message manager @and ls msg
 * @when parsing it with a ParseConstMsg
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseConstLs) {
  auto parsable_ls_msg = Buffer{}.put("ls\n");
  auto msg_opt = MessageManager::parseConstantMsg(parsable_ls_msg);
  ASSERT_TRUE(msg_opt);
  ASSERT_EQ(msg_opt.value().type, MessageType::LS);
}

/**
 * @given message manager @and na msg
 * @when parsing it with a ParseConstMsg
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseConstNa) {
  auto parsable_na_msg = Buffer{}.put("na\n");
  auto msg_opt = MessageManager::parseConstantMsg(parsable_na_msg);
  ASSERT_TRUE(msg_opt);
  ASSERT_EQ(msg_opt.value().type, MessageType::NA);
}

/**
 * @given message manager @and protocol msg
 * @when parsing it with a ParseConstMsg
 * @then parse fails
 */
TEST_F(MessageManagerTest, ParseConstFail) {
  EXPECT_FALSE(MessageManager::parseConstantMsg(kProtocolMsg.toVector()));
}

/**
 * @given message manager @and part of message with protocols header
 * @when parsing it
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseProtocolsHeader) {
  auto protocols_header = gsl::make_span(kProtocolsMsg.toVector());
  EXPECT_OUTCOME_TRUE(
      parsed_header,
      MessageManager::parseProtocolsHeader(protocols_header.subspan(1)))
  ASSERT_EQ(parsed_header.number_of_protocols, kProtocolsNumber);
  ASSERT_EQ(parsed_header.size_of_protocols, kProtocolsListBytesSize);
}

/**
 * @given message manager @and part of message with protocols
 * @when parsing it
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseProtocols) {
  auto protocols = gsl::make_span(kProtocolsMsg.toVector());
  EXPECT_OUTCOME_TRUE(
      parsed_protocols,
      MessageManager::parseProtocols(protocols.subspan(4), kProtocolsNumber))
  ASSERT_EQ(parsed_protocols.type, MessageType::PROTOCOLS);
  ASSERT_EQ(parsed_protocols.protocols, kDefaultProtocols);
}

/**
 * @given message manager @and protocol msg
 * @when parsing it as a protocols message
 * @then parse fails
 */
TEST_F(MessageManagerTest, ParseProtocolsFail) {
  EXPECT_FALSE(MessageManager::parseProtocols(kProtocolMsg.toVector(),
                                              kProtocolsNumber));
}

/**
 * @given message manager @and protocol msg
 * @when parsing it
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseProtocol) {
  auto protocol = gsl::make_span(kProtocolMsg.toVector());
  EXPECT_OUTCOME_TRUE(parsed_protocol,
                      MessageManager::parseProtocol(protocol.subspan(1)))
  ASSERT_EQ(parsed_protocol.type, MessageType::PROTOCOL);
  ASSERT_EQ(parsed_protocol.protocols[0], kDefaultProtocols[0]);
}

/**
 * @given message manager @and opening msg
 * @when parsing it
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseOpening) {
  auto opening = gsl::make_span(kOpeningMsg.toVector());
  EXPECT_OUTCOME_TRUE(parsed_protocol,
                      MessageManager::parseProtocol(opening.subspan(1)))
  ASSERT_EQ(parsed_protocol.type, MessageType::OPENING);
}
