/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "libp2p/network/connection_manager.hpp"
#include "libp2p/network/impl/connection_manager_impl.hpp"
#include "libp2p/peer/errors.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/network/transport_manager_mock.hpp"
#include "mock/libp2p/transport/transport_mock.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/literals.hpp"

using namespace libp2p::network;
using namespace libp2p::transport;
using namespace libp2p::connection;
using namespace libp2p::peer;
using namespace libp2p;

using testing::_;
using testing::NiceMock;
using testing::Return;
using C = ConnectionManager::Connectedness;

struct ConnectionManagerTest : public ::testing::Test {
  void SetUp() override {
    t = std::make_shared<TransportMock>();
    tmgr = std::make_shared<TransportManagerMock>();

    cmgr = std::make_shared<ConnectionManagerImpl>(tmgr);

    conn = std::make_shared<CapableConnectionMock>();

    // given 3 peers. p1 has 2 conns, p2 has 1, p3 has 0
    cmgr->addConnectionToPeer(p1, conn);
    cmgr->addConnectionToPeer(p1, conn);
    cmgr->addConnectionToPeer(p2, conn);
  }

  std::shared_ptr<TransportManagerMock> tmgr;
  std::shared_ptr<TransportMock> t;

  std::shared_ptr<ConnectionManager> cmgr;

  peer::PeerId p1 = testutil::randomPeerId();
  peer::PeerId p2 = testutil::randomPeerId();
  peer::PeerId p3 = testutil::randomPeerId();

  std::shared_ptr<CapableConnectionMock> conn;
};

/**
 * @given 3 peers. p1 has 2 conns, p2 has 1, p3 has 0
 * @when get all connections
 * @then get 4 connections
 */
TEST_F(ConnectionManagerTest, GetAllConnections) {
  auto c = cmgr->getConnections();
  ASSERT_EQ(c.size(), 3);
}

/**
 * @given 3 peers. p1 has 2 conns, p2 has 1, p3 has 0
 * @when get connections of specific peer
 * @then according number of connections is returned
 */
TEST_F(ConnectionManagerTest, GetConnToPeer) {
  auto c1 = cmgr->getConnectionsToPeer(p1);
  ASSERT_EQ(c1.size(), 2);

  auto c2 = cmgr->getConnectionsToPeer(p2);
  ASSERT_EQ(c2.size(), 1);

  auto c3 = cmgr->getConnectionsToPeer(p3);
  ASSERT_EQ(c3.size(), 0);
}

/**
 * @given 3 peers. p1 has 2 conns, p2 has 1, p3 has 0
 * @when get best connection
 * @then get valid connection
 */
TEST_F(ConnectionManagerTest, GetBestConn) {
  auto c = cmgr->getBestConnectionForPeer(p1);
  ASSERT_NE(c, nullptr);
}

/**
 * @given Peer with 2 valid connections
 * @when get its connections
 * @then get 2 valid connections, and connectedness status is CONNECTED
 */
TEST_F(ConnectionManagerTest, ConnectednessWhenConnected) {
  auto conns = cmgr->getConnectionsToPeer(p1);

  // even though there are 2 connections, we were able to find first open
  // connection, so isClosed called once
  EXPECT_CALL(*conn, isClosed()).WillOnce(Return(false));

  ASSERT_EQ(conns.size(), 2);

  ASSERT_NE(conns[0], nullptr);
  ASSERT_NE(conns[1], nullptr);

  ASSERT_EQ(cmgr->connectedness({p1, {}}), C::CONNECTED);
}

/**
 * @given peer with single nullptr connection
 * @when get connectedness
 * @then get NOT_CONNECTED
 */
TEST_F(ConnectionManagerTest, ConnectednessWhenNotConnected) {
  // simulate situation when connection was removed, but nullptr was left behind
  cmgr->addConnectionToPeer(p3, nullptr);

  // no valid connections
  ASSERT_EQ(cmgr->connectedness({p3, {}}), C::NOT_CONNECTED);
}

/**
 * @given peer without valid connections, but with valid address and transport
 * capable of dialing to this address
 * @when get connectedness to this peer
 * @then get CAN_CONNECT
 */
TEST_F(ConnectionManagerTest, ConnectednessWhenCanConnect) {
  auto ma = "/ip4/192.168.1.2/tcp/8080"_multiaddr;

  EXPECT_CALL(*tmgr, findBest(_)).WillOnce(Return(t));

  ASSERT_EQ(cmgr->connectedness({p3, {ma}}), C::CAN_CONNECT);
}

/**
 * @given peer has no connections and has no addresses
 * @when get connectedness
 * @then get CAN_NOT_CONNECT
 */
TEST_F(ConnectionManagerTest, ConnectednessWhenCanNotConnect_NoAddresses) {
  ASSERT_EQ(cmgr->connectedness({p3, {}}), C::CAN_NOT_CONNECT);
}

/**
 * @given peer has no connections, has address but has no transports that can
 * dial to that address
 * @when get connectedness
 * @then get CAN_NOT_CONNECT
 */
TEST_F(ConnectionManagerTest, ConnectednessWhenCanNotConnect) {
  auto ma = "/ip4/192.168.1.2/tcp/8080"_multiaddr;

  EXPECT_CALL(*tmgr, findBest(_)).WillOnce(Return(nullptr));

  ASSERT_EQ(cmgr->connectedness({p3, {ma}}), C::CAN_NOT_CONNECT);
}

/**
 * @given 3 peers: p1 has 2 closed connections, p1 has 1 closed connection, p3
 * has 1 nullptr connection
 * @when garbage colection is executed
 * @then all invalid connections are cleaned up
 */
TEST_F(ConnectionManagerTest, GarbageCollection) {
  cmgr->addConnectionToPeer(p3, nullptr);
  ASSERT_EQ(cmgr->getConnectionsToPeer(p1).size(), 2);
  ASSERT_EQ(cmgr->getConnectionsToPeer(p2).size(), 1);
  ASSERT_EQ(cmgr->getConnectionsToPeer(p3).size(), 1);

  EXPECT_CALL(*conn, isClosed()).Times(3).WillRepeatedly(Return(true));

  cmgr->collectGarbage();

  // since all connections are marked as "closed" or "nullptr", they all will be
  // garbage collected
  ASSERT_EQ(cmgr->getConnectionsToPeer(p1).size(), 0);
  ASSERT_EQ(cmgr->getConnectionsToPeer(p2).size(), 0);
  ASSERT_EQ(cmgr->getConnectionsToPeer(p3).size(), 0);
}
