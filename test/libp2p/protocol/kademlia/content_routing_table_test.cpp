/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "libp2p/protocol/kademlia/impl/content_routing_table_impl.hpp"

#include <gtest/gtest.h>
#include <unordered_set>

#include <libp2p/common/literals.hpp>
#include "mock/libp2p/basic/scheduler_mock.hpp"
#include "testutil/libp2p/peer.hpp"

using namespace libp2p;
using namespace protocol;
using namespace kademlia;
using namespace peer;
using namespace common;
using libp2p::event::Bus;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::ReturnRef;

struct ContentRoutingTableTest : public ::testing::Test {
  void SetUp() override {
    config_ = std::make_unique<Config>();

    scheduler_ = std::make_shared<basic::SchedulerMock>();
    EXPECT_CALL(*scheduler_, scheduleImplMockCall(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(*scheduler_, nowMockCall()).Times(AnyNumber());
    EXPECT_CALL(*scheduler_, cancelMockCall(_)).Times(AnyNumber());

    bus_ = std::make_shared<Bus>();

    table_ =
        std::make_unique<ContentRoutingTableImpl>(*config_, *scheduler_, bus_);
  }

  std::unique_ptr<Config> config_;
  std::shared_ptr<basic::SchedulerMock> scheduler_;
  std::shared_ptr<Bus> bus_;
  std::unique_ptr<ContentRoutingTable> table_;
  PeerId self_id = "1"_peerid;
  ContentId cid = ContentId{"content_key"};
};

template <typename A>
bool hasPeer(A &peerset, PeerId &peer) {
  return peerset.find(peer) != peerset.end();
}

TEST_F(ContentRoutingTableTest, BusWorks) {
  srand(0);  // to make test deterministic

  auto &provideCh =
      bus_->getChannel<event::protocol::kademlia::ProvideContentChannel>();

  std::unordered_set<PeerId> peerset;

  auto provideHandle = provideCh.subscribe(
      [&](std::pair<const ContentId &, const PeerId &> data) {
        peerset.insert(data.second);
      });

  std::vector<PeerId> peers;
  std::generate_n(std::back_inserter(peers), 1, testutil::randomPeerId);

  // table does not contain peer[0]
  table_->addProvider(cid, peers[0]);
  ASSERT_TRUE(hasPeer(peerset, peers[0])) << "should have this peer";
}

TEST_F(ContentRoutingTableTest, Provide) {
  config_->maxProvidersPerKey = 10;
  srand(0);  // to make test deterministic

  auto found_in_empty = table_->getProvidersFor(cid, 20);
  ASSERT_EQ(found_in_empty.size(), 0);

  size_t prev_count = 0;
  for (size_t i = 0; i < 20; ++i) {
    PeerId peer = testutil::randomPeerId();
    table_->addProvider(cid, peer);

    for (size_t limit = 1; limit <= 20; ++limit) {
      auto found = table_->getProvidersFor(cid, limit);
      ASSERT_LE(found.size(), std::min(limit, config_->maxProvidersPerKey));
      if (limit == 20) {
        ASSERT_GE(found.size(), prev_count);
      }
    }
  }
}
