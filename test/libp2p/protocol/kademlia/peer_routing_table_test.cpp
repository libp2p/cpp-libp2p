/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "libp2p/protocol/kademlia/impl/peer_routing_table_impl.hpp"

#include <gtest/gtest.h>
#include <unordered_set>

#include <libp2p/common/literals.hpp>
#include "mock/libp2p/peer/identity_manager_mock.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace libp2p;
using namespace protocol;
using namespace kademlia;
using namespace peer;
using namespace common;
using libp2p::event::Bus;

using ::testing::Return;
using ::testing::ReturnRef;

struct PeerRoutingTableTest : public ::testing::Test {
  void SetUp() override {
    testutil::prepareLoggers();

    config_ = std::make_unique<Config>();

    idmgr_ = std::make_shared<IdentityManagerMock>();
    EXPECT_CALL(*idmgr_, getId()).WillRepeatedly(ReturnRef(self_id));

    bus_ = std::make_shared<Bus>();

    table_ = std::make_unique<PeerRoutingTableImpl>(*config_, idmgr_, bus_);
  }

  std::unique_ptr<Config> config_;
  std::shared_ptr<IdentityManagerMock> idmgr_;
  std::shared_ptr<Bus> bus_;
  std::unique_ptr<PeerRoutingTable> table_;
  PeerId self_id = "1"_peerid;
};

template <typename A>
bool hasPeer(A &peerset, PeerId &peer) {
  return peerset.find(peer) != peerset.end();
}

TEST_F(PeerRoutingTableTest, BusWorks) {
  srand(0);  // to make test deterministic

  auto &addCh = bus_->getChannel<event::protocol::kademlia::PeerAddedChannel>();
  auto &remCh =
      bus_->getChannel<event::protocol::kademlia::PeerRemovedChannel>();

  std::unordered_set<PeerId> peerset;

  auto addHandle =
      addCh.subscribe([&](const PeerId &pid) { peerset.insert(pid); });

  auto removeHandle = remCh.subscribe([&](const PeerId &pid) {
    auto it = peerset.find(pid);
    ASSERT_TRUE(it != peerset.end());
    peerset.erase(it);
  });

  std::vector<PeerId> peers;
  std::generate_n(std::back_inserter(peers), 1, testutil::randomPeerId);

  // table does not contain peer[0]
  EXPECT_OUTCOME_TRUE_1(table_->update(peers[0], false));
  ASSERT_TRUE(hasPeer(peerset, peers[0])) << "should have this peer";

  table_->remove(peers[0]);
  ASSERT_FALSE(hasPeer(peerset, peers[0])) << "sholdn't have this peer";
}

/**
 * @brief
 *
 * @see
 * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/table_test.go#L168
 */
TEST_F(PeerRoutingTableTest, FindMultiple) {
  srand(0);  // to make test deterministic

  std::vector<PeerId> peers;
  std::generate_n(std::back_inserter(peers), 18, testutil::randomPeerId);

  for (const auto &peer : peers) {
    ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peer, false));
  }

  auto found = table_->getNearestPeers(NodeId(peers[2]), 15);
  ASSERT_EQ(found.size(), 15);
}

/**
 * @brief
 *
 * @see
 * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/table_test.go
 */
TEST_F(PeerRoutingTableTest, RecyclingTest) {
  config_->maxBucketSize = 1;
  srand(0);  // to make test deterministic
  auto &addCh = bus_->getChannel<event::protocol::kademlia::PeerAddedChannel>();
  auto &remCh =
      bus_->getChannel<event::protocol::kademlia::PeerRemovedChannel>();

  std::unordered_set<PeerId> peerset;

  auto addHandle =
      addCh.subscribe([&](const PeerId &pid) { peerset.insert(pid); });

  auto removeHandle = remCh.subscribe([&](const PeerId &pid) {
    auto it = peerset.find(pid);
    ASSERT_TRUE(it != peerset.end());
    peerset.erase(it);
  });

  std::vector<PeerId> peers;

  // Generate peers for first bucket, in count more than bucket capacity
  for (int i = 0; i < 3; ++i) {
    auto peer_id = testutil::randomPeerId();
    NodeId node_id(peer_id);
    if (node_id.commonPrefixLen(NodeId(self_id)) == 0) {
      peers.push_back(peer_id);
    }
  }

  ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[0], false));
  ASSERT_TRUE(hasPeer(peerset, peers[0])) << "should have this peer";

  ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[1], false));
  ASSERT_FALSE(hasPeer(peerset, peers[0])) << "should have recycled peer";
  ASSERT_TRUE(hasPeer(peerset, peers[1])) << "should have this peer";

  ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[2], true));
  ASSERT_FALSE(hasPeer(peerset, peers[0])) << "should have recycled peer";
  ASSERT_FALSE(hasPeer(peerset, peers[1])) << "should have recycled peer";
  ASSERT_TRUE(hasPeer(peerset, peers[2])) << "should have this peer";

  // if bucket is full of permanent peers addition of peers into same bucket
  // should fail
  ASSERT_OUTCOME_ERROR(table_->update(peers[0], false),
                       PeerRoutingTableImpl::Error::PEER_REJECTED_NO_CAPACITY);
  ASSERT_OUTCOME_ERROR(table_->update(peers[1], true),
                       PeerRoutingTableImpl::Error::PEER_REJECTED_NO_CAPACITY);

  // re-adding an existent peer should return false regardless of is permanent
  auto updateVal = table_->update(peers[2], true);
  ASSERT_OUTCOME_SUCCESS_TRY(updateVal);
  ASSERT_FALSE(updateVal.value());
  updateVal = table_->update(peers[2], false);
  ASSERT_OUTCOME_SUCCESS_TRY(updateVal);
  ASSERT_FALSE(updateVal.value());
}

TEST_F(PeerRoutingTableTest, PreferLongLivedPeers) {
  config_->maxBucketSize = 2;
  srand(0);  // to make test deterministic
  auto &addCh = bus_->getChannel<event::protocol::kademlia::PeerAddedChannel>();
  auto &remCh =
      bus_->getChannel<event::protocol::kademlia::PeerRemovedChannel>();

  std::unordered_set<PeerId> peerset;

  auto addHandle =
      addCh.subscribe([&](const PeerId &pid) { peerset.insert(pid); });

  auto removeHandle = remCh.subscribe([&](const PeerId &pid) {
    auto it = peerset.find(pid);
    ASSERT_TRUE(it != peerset.end());
    peerset.erase(it);
  });

  std::vector<PeerId> peers;

  // Generate peers for first bucket, in count more than bucket capacity
  while (peers.size() != 3) {
    auto peer_id = testutil::randomPeerId();
    NodeId node_id(peer_id);
    if (node_id.commonPrefixLen(NodeId(self_id)) == 0) {
      peers.push_back(peer_id);
    }
  }
  // recycle FIFO; known peers but not connected dont get boost
  ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[0], false));
  ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[1], false));
  ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[0], false));
  ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[2], false));

  ASSERT_FALSE(hasPeer(peerset, peers[0]));
  ASSERT_TRUE(hasPeer(peerset, peers[1]));
  ASSERT_TRUE(hasPeer(peerset, peers[2]));

  // if connected; peer gets a boost
  ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[1], false, true));
  ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[0], false));

  ASSERT_TRUE(hasPeer(peerset, peers[0]));
  ASSERT_TRUE(hasPeer(peerset, peers[1]));
  ASSERT_FALSE(hasPeer(peerset, peers[2]));
}

TEST_F(PeerRoutingTableTest, EldestRecycledIfNotPermanent) {
  config_->maxBucketSize = 3;
  srand(0);  // to make test deterministic

  std::vector<PeerId> peers;

  // Generate peers for first bucket, in count more than bucket capacity
  while (peers.size() != config_->maxBucketSize * 2) {
    auto peer_id = testutil::randomPeerId();
    NodeId node_id(peer_id);
    if (node_id.commonPrefixLen(NodeId(self_id)) == 0) {
      peers.push_back(peer_id);
    }
  }

  // Try to add all of peer to
  for (size_t i = 0; i < peers.size(); i++) {
    if (i < config_->maxBucketSize) {
      // peers added till bucket filled are accepted
      ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[i], true));
    } else {
      // Remained are rejected
      ASSERT_OUTCOME_ERROR(
          table_->update(peers[i], true),
          PeerRoutingTableImpl::Error::PEER_REJECTED_NO_CAPACITY);
    }
  }
}

TEST_F(PeerRoutingTableTest, EldestPrefferedIfPermanent) {
  config_->maxBucketSize = 3;
  srand(0);  // to make test deterministic

  std::vector<PeerId> peers;

  // Generate peers for first bucket, in count more than bucket capacity
  while (peers.size() != config_->maxBucketSize * 2) {
    auto peer_id = testutil::randomPeerId();
    NodeId node_id(peer_id);
    if (node_id.commonPrefixLen(NodeId(self_id)) == 0) {
      peers.push_back(peer_id);
    }
  }

  // Try to add all of peer to
  for (size_t i = 0; i < peers.size(); i++) {
    if (i < config_->maxBucketSize) {
      // peers added till bucket filled are accepted
      ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peers[i], true));
    } else {
      // Remained are rejected
      ASSERT_OUTCOME_ERROR(
          table_->update(peers[i], true),
          PeerRoutingTableImpl::Error::PEER_REJECTED_NO_CAPACITY);
    }
  }
}

/**
 * @see
 * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/table_test.go#L97
 */
TEST_F(PeerRoutingTableTest, Update) {
  config_->maxBucketSize = 10;
  srand(0);  // to make test deterministic

  std::vector<PeerId> peers;
  std::generate_n(std::back_inserter(peers), 100, testutil::randomPeerId);

  // 10000 random updates among 100 existing peers
  for (int i = 0; i < 10000; i++) {
    int index = rand() % peers.size();
    [[maybe_unused]] auto result = table_->update(peers[index], false);
  }

  size_t total_peers = table_->size();
  size_t count = 5;
  size_t expected_count = std::min(count, total_peers);

  for (int i = 0; i < 100; i++) {
    auto found =
        table_->getNearestPeers(NodeId(testutil::randomPeerId()), count);
    ASSERT_EQ(found.size(), expected_count);
  }
}

/**
 * @see
 * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/table_test.go#L121:1
 */
TEST_F(PeerRoutingTableTest, Find) {
  config_->maxBucketSize = 10;
  srand(0);  // to make test deterministic

  const auto nPeers = 5;

  std::vector<PeerId> peers;
  std::generate_n(std::back_inserter(peers), nPeers, testutil::randomPeerId);

  for (const auto &peer : peers) {
    ASSERT_OUTCOME_SUCCESS_TRY(table_->update(peer, false));
  }
  ASSERT_EQ(table_->size(), nPeers);

  for (const auto &peer : peers) {
    auto found = table_->getNearestPeers(NodeId(peer), 1);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].toHex(), peer.toHex()) << "failed to lookup known node";
  }
}
