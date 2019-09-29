/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "libp2p/protocol/kademlia/impl/routing_table_impl.hpp"

#include <gtest/gtest.h>
#include <unordered_set>

#include "mock/libp2p/peer/identity_manager_mock.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;
using namespace protocol;
using namespace kademlia;
using namespace peer;
using libp2p::event::Bus;

using ::testing::Return;
using ::testing::ReturnRef;

struct RoutingTableFixture : public ::testing::Test {
  RoutingTableFixture() {
    EXPECT_CALL(*idmgr, getId()).WillRepeatedly(ReturnRef(local));
    rt = std::make_shared<RoutingTableImpl>(idmgr, bus);
  }

  std::shared_ptr<Bus> bus = std::make_shared<Bus>();

  std::shared_ptr<IdentityManagerMock> idmgr =
      std::make_shared<IdentityManagerMock>();

  std::shared_ptr<RoutingTableImpl> rt;

  PeerId local = "1"_peerid;
};

template <typename A>
bool hasPeer(A &peerset, PeerId &peer) {
  return peerset.find(peer) != peerset.end();
}

TEST_F(RoutingTableFixture, BusWorks) {
  srand(0);  // to make test deterministic

  auto &addCh = bus->getChannel<events::PeerAddedChannel>();
  auto &remCh = bus->getChannel<events::PeerRemovedChannel>();

  std::unordered_set<peer::PeerId> peerset;

  auto addHandle = addCh.subscribe([&](const peer::PeerId &pid) {
    std::cout << "added: " << pid.toHex() << "\n";
    peerset.insert(pid);
  });

  auto removeHandle = remCh.subscribe([&](const peer::PeerId &pid) {
    std::cout << "removed: " << pid.toHex() << "\n";
    auto it = peerset.find(pid);
    ASSERT_TRUE(it != peerset.end());
    peerset.erase(it);
  });

  std::vector<PeerId> peers;
  std::generate_n(
      std::back_inserter(peers), 1, []() { return testutil::randomPeerId(); });

  // table does not contain peer[0]
  EXPECT_OUTCOME_TRUE_1(rt->update(peers[0]));
  ASSERT_TRUE(hasPeer(peerset, peers[0])) << "should have this peer";

  rt->remove(peers[0]);
  ASSERT_FALSE(hasPeer(peerset, peers[0])) << "sholdn't have this peer";
}

/**
 * @brief
 *
 * @see
 * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/table_test.go#L168
 */
TEST_F(RoutingTableFixture, FindMultiple) {
  rt = std::make_shared<RoutingTableImpl>(idmgr, bus, RoutingTable::Config{20});
  srand(0);  // to make test deterministic

  std::vector<PeerId> peers;
  std::generate_n(
      std::back_inserter(peers), 18, []() { return testutil::randomPeerId(); });

  for (const auto &peer : peers) {
    EXPECT_OUTCOME_TRUE_1(rt->update(peer));
  }

  auto found = rt->getNearestPeers(NodeId(peers[2]), 15);
  ASSERT_EQ(found.size(), 15);
}

/**
 * @brief
 *
 * @see
 * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/table_test.go
 */
TEST_F(RoutingTableFixture, EldestPreferred) {
  rt = std::make_shared<RoutingTableImpl>(idmgr, bus, RoutingTable::Config{10});
  srand(0);  // to make test deterministic

  std::vector<PeerId> peers;
  // generate size + 1 peers to saturate a bucket
  while (peers.size() != 15) {
    auto id = testutil::randomPeerId();
    auto nid = NodeId(id);
    if (nid.commonPrefixLen(NodeId(local)) == 0) {
      peers.push_back(id);
    }
  }

  // test 10 first peers are accepted.
  for (size_t i = 0; i < 10; i++) {
    EXPECT_OUTCOME_TRUE_1(rt->update(peers[i]));
  }

  // test next 5 peers are rejected.
  for (size_t i = 10; i < peers.size(); i++) {
    EXPECT_OUTCOME_FALSE(err, rt->update(peers[i]));
    ASSERT_EQ(err.value(),
              (int)RoutingTableImpl::Error::PEER_REJECTED_NO_CAPACITY);
  }
}

/**
 * @see
 * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/table_test.go#L97
 */
TEST_F(RoutingTableFixture, TableUpdate) {
  rt = std::make_shared<RoutingTableImpl>(idmgr, bus, RoutingTable::Config{10});
  srand(0);  // to make test deterministic

  std::vector<PeerId> peers;
  std::generate_n(std::back_inserter(peers), 100, []() {
    return testutil::randomPeerId();
  });

  // 10000 random updates among 100 existing peers
  for (int i = 0; i < 10000; i++) {
    int index = rand() % peers.size();
    (void)rt->update(peers[index]);
  }

  for (int i = 0; i < 100; i++) {
    auto found = rt->getNearestPeers(NodeId(testutil::randomPeerId()), 5);
    ASSERT_NE(found.size(), 0);
  }
}

/**
 * @see
 * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/table_test.go#L121:1
 */
TEST_F(RoutingTableFixture, TableFind) {
  const auto nPeers = 5;

  rt = std::make_shared<RoutingTableImpl>(idmgr, bus, RoutingTable::Config{10});
  srand(0);  // to make test deterministic

  std::vector<PeerId> peers;
  std::generate_n(
      std::back_inserter(peers), nPeers, []() { return testutil::randomPeerId(); });

  for (const auto &peer : peers) {
    EXPECT_OUTCOME_TRUE_1(rt->update(peer));
  }
  ASSERT_EQ(rt->size(), nPeers);

  for (const auto &peer : peers) {
    auto found = rt->getNearestPeers(NodeId(peer), 1);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].toHex(), peer.toHex()) << "failed to lookup known node";
  }
}
