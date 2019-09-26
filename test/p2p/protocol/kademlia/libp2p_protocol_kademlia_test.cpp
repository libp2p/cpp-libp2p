/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/kademlia/impl/kad_impl.hpp"

#include <gtest/gtest.h>

#include "mock/libp2p/network/connection_manager_mock.hpp"
#include "mock/libp2p/network/network_mock.hpp"
#include "mock/libp2p/peer/address_repository_mock.hpp"
#include "mock/libp2p/peer/peer_repository_mock.hpp"
#include "mock/libp2p/protocol/kademlia/message_read_writer_mock.hpp"
#include "mock/libp2p/protocol/kademlia/query_runner_mock.hpp"
#include "mock/libp2p/protocol/kademlia/routing_table_mock.hpp"

#include "testutil/gmock_actions.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;
using namespace protocol;
using namespace kademlia;
using namespace network;
using namespace peer;

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;

struct KadTest : public ::testing::Test {
  StrictMock<ConnectionManagerMock> cmgr;
  StrictMock<AddressRepositoryMock> addrrepo;
  StrictMock<NetworkMock> network;

  std::shared_ptr<peer::PeerRepositoryMock> repo =
      std::make_shared<peer::PeerRepositoryMock>();

  std::shared_ptr<RoutingTableMock> table =
      std::make_shared<RoutingTableMock>();
  std::shared_ptr<MessageReadWriterMock> mrw =
      std::make_shared<MessageReadWriterMock>();
  std::shared_ptr<QueryRunnerMock> runner = std::make_shared<QueryRunnerMock>();

  KademliaConfig config = {};

  std::shared_ptr<KadImpl> kad =
      std::make_shared<KadImpl>(network, repo, table, mrw, runner, config);

  // our peer
  peer::PeerId usid = "our peer"_peerid;
  // address of p1
  multi::Multiaddress usma = "/ip4/127.0.0.1/tcp/1337"_multiaddr;
  peer::PeerInfo us = {usid, {usma}};

  // other peers
  peer::PeerInfo peer1 = {"1"_peerid, {"/ip4/127.0.0.1/tcp/1"_multiaddr}};
  peer::PeerInfo peer2 = {"2"_peerid, {"/ip4/127.0.0.1/tcp/2"_multiaddr}};
  peer::PeerInfo peer3 = {"3"_peerid, {"/ip4/127.0.0.1/tcp/3"_multiaddr}};
  peer::PeerInfo peer4 = {"4"_peerid, {"/ip4/127.0.0.1/tcp/4"_multiaddr}};

  void executeTest(peer::PeerId p, peer::PeerInfo expected) {
    bool executed = false;
    kad->findPeer(p, [&](auto &&res) {
      EXPECT_OUTCOME_TRUE(pinfo, res);
      executed = true;
      ASSERT_EQ(pinfo, expected);
    });
    ASSERT_TRUE(executed);
  }
};

template <typename T>
outcome::result<T> makeFailure() {
  // we do not care about error code, we are interested in error condition
  // itself
  return std::errc::invalid_argument;
}

/**
 * @given kad network, 1 connection to 'usid'
 * @when FindPeer(usid)
 * @then get Connected; expect to get correct PeerInfo
 */
TEST_F(KadTest, FindPeerExists) {
  EXPECT_CALL(*repo, getAddressRepository()).WillOnce(ReturnRef(addrrepo));
  EXPECT_CALL(addrrepo, getAddresses(usid)).WillOnce(Return(us.addresses));
  EXPECT_CALL(network, getConnectionManager()).WillOnce(ReturnRef(cmgr));
  EXPECT_CALL(cmgr, connectedness(us))
      .WillOnce(Return(ConnectionManager::Connectedness::CONNECTED));

  executeTest(usid, us);
}

/**
 * @given kad network, 0 connections. peers 2,3,4 are in our routing table
 * @when FindPeer(peer 1 id)
 * @then find correct PeerInfo
 */
TEST_F(KadTest, FindPeerNoAddresses) {
  // we don't know address of peer 1, so lets try to find it out

  EXPECT_CALL(*repo, getAddressRepository())
      .WillRepeatedly(ReturnRef(addrrepo));
  EXPECT_CALL(addrrepo, getAddresses(peer1.id))
      .WillRepeatedly(Return(makeFailure<std::vector<multi::Multiaddress>>()));
  EXPECT_CALL(network, getConnectionManager()).WillOnce(ReturnRef(cmgr));

  // we are not connected to peer 1, since we don't know its addresses
  EXPECT_CALL(cmgr, connectedness(_))
      .WillOnce(Return(ConnectionManager::Connectedness::NOT_CONNECTED));

  // list of peers we observe (2,3,4)
  PeerIdVec weObserve{peer2.id, peer3.id, peer4.id};
  EXPECT_CALL(*table, getNearestPeers(_, _))
      .WillOnce(Return(weObserve));

  // run query... and get desired addr
  EXPECT_CALL(*runner, run(_, _, _)).WillOnce(Arg2CallbackWithArg(peer1));

  executeTest(peer1.id, peer1);
}
