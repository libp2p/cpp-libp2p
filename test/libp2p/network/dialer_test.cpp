/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/common/literals.hpp>
#include <libp2p/network/impl/dialer_impl.hpp>
#include <qtils/test/outcome.hpp>
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/network/connection_manager_mock.hpp"
#include "mock/libp2p/network/listener_mock.hpp"
#include "mock/libp2p/network/transport_manager_mock.hpp"
#include "mock/libp2p/peer/address_repository_mock.hpp"
#include "mock/libp2p/protocol_muxer/protocol_muxer_mock.hpp"
#include "mock/libp2p/transport/transport_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/prepare_loggers.hpp"

using libp2p::peer::AddressRepositoryMock;

using namespace libp2p;
using namespace network;
using namespace connection;
using namespace transport;
using namespace protocol_muxer;
using namespace common;
using namespace basic;

using ::testing::_;
using ::testing::ContainerEq;
using ::testing::Contains;
using ::testing::Eq;
using ::testing::InvokeArgument;
using ::testing::Return;
using ::testing::Truly;

struct DialerTest : public ::testing::Test {
  void SetUp() override {
    testutil::prepareLoggers();
    dialer = std::make_shared<DialerImpl>(
        proto_muxer, tmgr, cmgr, listener, addr_repo, scheduler);
  }

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

  std::shared_ptr<ListenerMock> listener = std::make_shared<ListenerMock>();

  std::shared_ptr<AddressRepositoryMock> addr_repo =
      std::make_shared<AddressRepositoryMock>();

  std::shared_ptr<ManualSchedulerBackend> scheduler_backend =
      std::make_shared<ManualSchedulerBackend>();

  std::shared_ptr<Scheduler> scheduler =
      std::make_shared<SchedulerImpl>(scheduler_backend, Scheduler::Config{});

  std::shared_ptr<Dialer> dialer;

  multi::Multiaddress ma1 = "/ip4/127.0.0.1/tcp/1"_multiaddr;
  multi::Multiaddress ma2 = "/ip4/127.0.0.1/tcp/2"_multiaddr;
  peer::PeerId pid = "1"_peerid;
  const StreamProtocols protocols = {"/protocol/1.0.0"};

  peer::PeerInfo pinfo{.id = pid, .addresses = {ma1}};
  peer::PeerInfo pinfo_two_addrs{.id = pid, .addresses = {ma1, ma2}};
};

/**
 * @given a peer with two multiaddresses
 * @when a dial to the first address fails
 * @then the dialer will try the second supplied address too
 */
TEST_F(DialerTest, DialAllTheAddresses) {
  // we dont have connection already
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pinfo.id))
      .WillOnce(Return(nullptr));

  // connection is stored
  EXPECT_CALL(*listener, onConnection(_)).Times(1);

  // we have transport to dial
  EXPECT_CALL(*tmgr, findBest(ma1)).WillOnce(Return(transport));
  EXPECT_CALL(*tmgr, findBest(ma2)).WillOnce(Return(transport));

  // transport->dial returns an error for the first address
  EXPECT_CALL(*transport, dial(pinfo_two_addrs.id, ma1, _))
      .WillOnce(
          Arg2CallbackWithArg(make_error_code(std::errc::connection_refused)));

  // transport->dial returns valid connection for the second address
  EXPECT_CALL(*transport, dial(pinfo_two_addrs.id, ma2, _))
      .WillOnce(Arg2CallbackWithArg(outcome::success(connection)));

  bool executed = false;
  dialer->dial(pinfo_two_addrs, [&](auto &&rconn) {
    ASSERT_OUTCOME_SUCCESS(conn, rconn);
    (void)conn;
    executed = true;
  });

  scheduler_backend->run();

  ASSERT_TRUE(executed);
}

/**
 * @given no known connections to peer, have 1 transport, 1 address supplied
 * @when dial
 * @then create new connection using transport
 */
TEST_F(DialerTest, DialNewConnection) {
  // we dont have connection already
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pinfo.id))
      .WillOnce(Return(nullptr));

  // connection is stored
  EXPECT_CALL(*listener, onConnection(_)).Times(1);

  // we have transport to dial
  EXPECT_CALL(*tmgr, findBest(ma1)).WillOnce(Return(transport));

  // transport->dial returns valid connection
  EXPECT_CALL(*transport, dial(pinfo.id, ma1, _))
      .WillOnce(Arg2CallbackWithArg(outcome::success(connection)));

  bool executed = false;
  dialer->dial(pinfo, [&](auto &&rconn) {
    ASSERT_OUTCOME_SUCCESS(conn, rconn);
    (void)conn;
    executed = true;
  });

  scheduler_backend->run();

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
    ASSERT_OUTCOME_ERROR(rconn, std::errc::destination_address_required);
    executed = true;
  });

  scheduler_backend->run();

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
    ASSERT_OUTCOME_ERROR(rconn, std::errc::address_family_not_supported);
    executed = true;
  });

  scheduler_backend->run();

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
    ASSERT_OUTCOME_SUCCESS(conn, rconn);
    (void)conn;
    executed = true;
  });

  scheduler_backend->run();

  ASSERT_TRUE(executed);
}

///
/// All tests that use newStream assume connections already exist, because
/// newStream uses dial to get connection, and dial is already tested for all
/// cases.
///

/**
 * @given no connections to peer
 * @when newStream is executed
 * @then get failure
 */
TEST_F(DialerTest, NewStreamFailed) {
  // no existing connections to peer
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pid))
      .WillOnce(Return(connection));

  // report random error.
  // we simulate a case when "newStream" gets error
  EXPECT_CALL(*connection, newStream()).WillOnce(Return(std::errc::io_error));

  bool executed = false;
  dialer->newStream(pinfo, protocols, [&](auto &&rstream) {
    ASSERT_OUTCOME_ERROR(rstream, std::errc::io_error);
    executed = true;
  });

  scheduler_backend->run();

  ASSERT_TRUE(executed);
}

/**
 * @given existing connection to peer
 * @when newStream is executed
 * @then get negotiation failure
 */
TEST_F(DialerTest, NewStreamNegotiationFailed) {
  // connection exists to peer
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pid))
      .WillOnce(Return(connection));

  // newStream returns valid stream
  EXPECT_CALL(*connection, newStream()).WillOnce(Return(stream));

  auto r = std::errc::io_error;

  auto if_protocols = [&](std::span<const peer::ProtocolName> actual) {
    auto expected = std::span<const peer::ProtocolName>(protocols);
    return std::equal(
        actual.begin(), actual.end(), expected.begin(), expected.end());
  };

  EXPECT_CALL(*proto_muxer, selectOneOf(Truly(if_protocols), _, _, _, _))
      .WillOnce(InvokeArgument<4>(make_error_code(r)));

  bool executed = false;
  dialer->newStream(pinfo, protocols, [&](auto &&rstream) {
    ASSERT_OUTCOME_ERROR(rstream, r);
    executed = true;
  });

  scheduler_backend->run();

  ASSERT_TRUE(executed);
}

/**
 * @given existing connection to peer
 * @when newStream is executed
 * @then get new stream
 */
TEST_F(DialerTest, NewStreamSuccess) {
  // connection exists to peer
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pid))
      .WillOnce(Return(connection));

  // newStream returns valid stream
  EXPECT_CALL(*connection, newStream()).WillOnce(Return(stream));

  auto if_protocols = [&](std::span<const peer::ProtocolName> actual) {
    auto expected = std::span<const peer::ProtocolName>(protocols);
    return std::equal(
        actual.begin(), actual.end(), expected.begin(), expected.end());
  };

  EXPECT_CALL(*proto_muxer, selectOneOf(Truly(if_protocols), _, _, _, _))
      .WillOnce(InvokeArgument<4>(protocols[0]));

  bool executed = false;
  dialer->newStream(pinfo, protocols, [&](auto &&rstream) {
    ASSERT_OUTCOME_SUCCESS(s, rstream);
    (void)s;
    executed = true;
  });

  scheduler_backend->run();

  ASSERT_TRUE(executed);
}
