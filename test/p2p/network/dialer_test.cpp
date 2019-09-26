/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/network/impl/dialer_impl.hpp"

#include <gtest/gtest.h>
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/network/connection_manager_mock.hpp"
#include "mock/libp2p/network/router_mock.hpp"
#include "mock/libp2p/network/transport_manager_mock.hpp"
#include "mock/libp2p/peer/address_repository_mock.hpp"
#include "mock/libp2p/protocol_muxer/protocol_muxer_mock.hpp"
#include "mock/libp2p/transport/transport_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;
using namespace network;
using namespace connection;
using namespace transport;
using namespace protocol_muxer;

using ::testing::_;
using ::testing::ContainerEq;
using ::testing::Contains;
using ::testing::Eq;
using ::testing::Return;

struct DialerTest : public ::testing::Test {
  std::shared_ptr<StreamMock> stream = std::make_shared<StreamMock>();

  std::shared_ptr<CapableConnectionMock> connection =
      std::make_shared<CapableConnectionMock>();

  std::shared_ptr<TransportMock> transport = std::make_shared<TransportMock>();

  std::shared_ptr<ProtocolMuxerMock> proto_muxer =
      std::make_shared<ProtocolMuxerMock>();

  std::shared_ptr<TransportManagerMock> tmgr =
      std::make_shared<TransportManagerMock>();

  std::shared_ptr<ConnectionManagerMock> cmgr =
      std::make_shared<ConnectionManagerMock>();

  std::shared_ptr<Dialer> dialer =
      std::make_shared<DialerImpl>(proto_muxer, tmgr, cmgr);

  multi::Multiaddress ma1 = "/ip4/127.0.0.1/tcp/1"_multiaddr;
  peer::PeerId pid = "1"_peerid;
  peer::Protocol protocol = "/protocol/1.0.0";

  peer::PeerInfo pinfo{.id = pid, .addresses = {ma1}};
};

/**
 * @given no known connections to peer, have 1 transport, 1 address supplied
 * @when dial
 * @then create new connection using transport
 */
TEST_F(DialerTest, DialNewConnection) {
  // we dont have connection already
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pinfo.id))
      .WillOnce(Return(nullptr));

  // we have transport to dial
  EXPECT_CALL(*tmgr, findBest(ma1)).WillOnce(Return(transport));

  // transport->dial returns valid connection
  EXPECT_CALL(*transport, dial(pinfo.id, ma1, _))
      .WillOnce(Arg2CallbackWithArg(outcome::success(connection)));

  // connection is stored by connection manager
  EXPECT_CALL(*cmgr, addConnectionToPeer(pinfo.id, _)).Times(1);

  bool executed = false;
  dialer->dial(pinfo, [&](auto &&rconn) {
    EXPECT_OUTCOME_TRUE(conn, rconn);
    (void)conn;
    executed = true;
  });

  ASSERT_TRUE(executed);
}

/**
 * @given no known connections to peer, no addresses supplied
 * @when dial
 * @then create new connection using transport
 */
TEST_F(DialerTest, DialNoAddresses) {
  // we dont have connection already
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pinfo.id))
      .WillOnce(Return(nullptr));

  // no addresses supplied
  peer::PeerInfo pinfo = {pid, {}};
  bool executed = false;
  dialer->dial(pinfo, [&](auto &&rconn) {
    EXPECT_OUTCOME_FALSE(e, rconn);
    EXPECT_EQ(e.value(), (int)std::errc::destination_address_required);
    executed = true;
  });

  ASSERT_TRUE(executed);
}

/**
 * @given no known connections to peer, have 1 tcp transport, 1 UDP address
 * supplied
 * @when dial
 * @then can not dial, no transports found
 */
TEST_F(DialerTest, DialNoTransports) {
  // we dont have connection already
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pinfo.id))
      .WillOnce(Return(nullptr));

  // we did not find transport to dial
  EXPECT_CALL(*tmgr, findBest(ma1)).WillOnce(Return(nullptr));

  bool executed = false;
  dialer->dial(pinfo, [&](auto &&rconn) {
    EXPECT_OUTCOME_FALSE(e, rconn);
    EXPECT_EQ(e.value(), (int)std::errc::address_family_not_supported);
    executed = true;
  });

  ASSERT_TRUE(executed);
}

/**
 * @given existing connection to peer
 * @when dial
 * @then get existing connection
 */
TEST_F(DialerTest, DialExistingConnection) {
  // we have connection
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pinfo.id))
      .WillOnce(Return(connection));

  bool executed = false;
  dialer->dial(pinfo, [&](auto &&rconn) {
    EXPECT_OUTCOME_TRUE(conn, rconn);
    (void)conn;
    executed = true;
  });

  ASSERT_TRUE(executed);
}

///
/// All tests that use newStream assume connections already exist, because
/// newStream uses dial to get connection, and dial is already tested for all
/// cases.
///

/**
 * @given existing connection to peer
 * @when newStream is executed
 * @then get failure
 */
TEST_F(DialerTest, NewStreamFailed) {
  // no existing connections to peer
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pid))
      .WillOnce(Return(connection));

  // report random error.
  // we simulate a case when "newStream" gets error
  outcome::result<std::shared_ptr<Stream>> r = std::errc::io_error;
  EXPECT_CALL(*connection, newStream(_)).WillOnce(Arg0CallbackWithArg(r));

  bool executed = false;
  dialer->newStream(pinfo, protocol, [&](auto &&rstream) {
    EXPECT_OUTCOME_FALSE(e, rstream);
    EXPECT_EQ(e.value(), (int)std::errc::io_error);
    executed = true;
  });
  ASSERT_TRUE(executed);
}

/**
 * @given existing connection to peer
 * @when newStream is executed
 * @then get negotiation failure
 */
TEST_F(DialerTest, NewStreamNegotiationFailed) {
  // connection exist to peer
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pid))
      .WillOnce(Return(connection));

  // newStream returns valid stream
  EXPECT_CALL(*connection, newStream(_)).WillOnce(Arg0CallbackWithArg(stream));

  outcome::result<peer::Protocol> r = std::errc::io_error;
  EXPECT_CALL(*proto_muxer, selectOneOf(Contains(Eq(protocol)), _, true, _))
      .WillOnce(Arg3CallbackWithArg(r));

  bool executed = false;
  dialer->newStream(pinfo, protocol, [&](auto &&rstream) {
    EXPECT_OUTCOME_FALSE(e, rstream);
    EXPECT_EQ(e.value(), (int)std::errc::io_error);
    executed = true;
  });
  ASSERT_TRUE(executed);
}

/**
 * @given existing connection to peer
 * @when newStream is executed
 * @then get new stream
 */
TEST_F(DialerTest, NewStreamSuccess) {
  // connection exist to peer
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pid))
      .WillOnce(Return(connection));

  // newStream returns valid stream
  EXPECT_CALL(*connection, newStream(_)).WillOnce(Arg0CallbackWithArg(stream));

  EXPECT_CALL(*proto_muxer, selectOneOf(Contains(Eq(protocol)), _, true, _))
      .WillOnce(Arg3CallbackWithArg(protocol));

  bool executed = false;
  dialer->newStream(pinfo, protocol, [&](auto &&rstream) {
    EXPECT_OUTCOME_TRUE(s, rstream);
    (void)s;
    executed = true;
  });
  ASSERT_TRUE(executed);
}
