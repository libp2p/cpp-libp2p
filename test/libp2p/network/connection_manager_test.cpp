/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <libp2p/common/literals.hpp>
#include "libp2p/network/connection_manager.hpp"
#include "libp2p/network/impl/connection_manager_impl.hpp"
#include "libp2p/peer/errors.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/transport/transport_mock.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace libp2p;
using namespace network;
using namespace transport;
using namespace connection;
using namespace peer;
using namespace common;

using testing::_;
using testing::NiceMock;
using testing::Return;

struct ConnectionManagerTest : public ::testing::Test {
  void SetUp() override {
    t = std::make_shared<TransportMock>();

    bus = std::make_shared<libp2p::event::Bus>();

    cmgr = std::make_shared<ConnectionManagerImpl>(bus);

    conn11 = std::make_shared<CapableConnectionMock>();
    conn12 = std::make_shared<CapableConnectionMock>();
    conn2 = std::make_shared<CapableConnectionMock>();

    // given 3 peers. p1 has 2 conns, p2 has 1, p3 has 0
    cmgr->addConnectionToPeer(p1, conn11);
    cmgr->addConnectionToPeer(p1, conn12);
    cmgr->addConnectionToPeer(p2, conn2);
  }

  std::shared_ptr<libp2p::event::Bus> bus;
  std::shared_ptr<TransportMock> t;

  std::shared_ptr<ConnectionManager> cmgr;

  peer::PeerId p1 = testutil::randomPeerId();
  peer::PeerId p2 = testutil::randomPeerId();
  peer::PeerId p3 = testutil::randomPeerId();

  std::shared_ptr<CapableConnectionMock> conn11;
  std::shared_ptr<CapableConnectionMock> conn12;
  std::shared_ptr<CapableConnectionMock> conn2;
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
  EXPECT_CALL(*conn11, isClosed()).WillRepeatedly(Return(false));
  EXPECT_CALL(*conn12, isClosed()).WillRepeatedly(Return(false));
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

  ASSERT_EQ(conns.size(), 2);

  ASSERT_NE(conns[0], nullptr);
  ASSERT_NE(conns[1], nullptr);
}

/**
 * @given 3 peers: p1 has 2 closed connections, p1 has 1 closed connection, p3
 * has 1 nullptr connection
 * @when garbage colection is executed
 * @then all invalid connections are cleaned up
 */
TEST_F(ConnectionManagerTest, GarbageCollection) {
  // should ignore nullptr!
  cmgr->addConnectionToPeer(p3, nullptr);

  ASSERT_EQ(cmgr->getConnectionsToPeer(p1).size(), 2);
  ASSERT_EQ(cmgr->getConnectionsToPeer(p2).size(), 1);
  ASSERT_EQ(cmgr->getConnectionsToPeer(p3).size(), 0);

  EXPECT_CALL(*conn11, isClosed()).WillOnce(Return(true));
  EXPECT_CALL(*conn12, isClosed()).WillOnce(Return(true));
  EXPECT_CALL(*conn2, isClosed()).WillOnce(Return(true));

  cmgr->collectGarbage();

  // since all connections are marked as "closed" or "nullptr", they all will be
  // garbage collected
  ASSERT_EQ(cmgr->getConnectionsToPeer(p1).size(), 0);
  ASSERT_EQ(cmgr->getConnectionsToPeer(p2).size(), 0);
  ASSERT_EQ(cmgr->getConnectionsToPeer(p3).size(), 0);
}

int main(int argc, char *argv[]) {
  if (std::getenv("TRACE_DEBUG") != nullptr) {
    testutil::prepareLoggers(soralog::Level::TRACE);
  } else {
    testutil::prepareLoggers(soralog::Level::ERROR);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
