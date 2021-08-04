/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "src/protocol/gossip/impl/message_cache.hpp"
#include "src/protocol/gossip/impl/peer_set.hpp"

#include <gtest/gtest.h>

#include "testutil/libp2p/peer.hpp"

namespace g = libp2p::protocol::gossip;

/**
 * @given An arbitrary TopicMessage
 * @when Decoding its fields
 * @then Fields appear to be sane: 'from' is a valid peer id, seq_no
 * encoded properly, message id is valid
 */
TEST(Gossip, TopicMessageHasValidFields) {
  auto peer = testutil::randomPeerId();

  auto msg = std::make_shared<g::TopicMessage>(
      peer, 0x2233445566778899ull, g::fromString("hahaha"), "topic");
  // peer is encoded properly
  auto peer_res = peerFrom(*msg);
  ASSERT_TRUE(peer_res);
  ASSERT_EQ(peer, peer_res.value());

  // seq_no is encoded properly
  ASSERT_EQ(msg->seq_no,
            g::ByteArray({0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}));

  // id is created from proper fields
  g::MessageId id = g::createMessageId(msg->from, msg->seq_no, msg->data);
  ASSERT_EQ(id.size(), 42);
}

/**
 * @given NP peers subscribed to NT topics in arbitrary manner
 * @when We insert them into PeerSet
 * @then We see that selectRandom, selectAll, eraseIf operations on PeerSet
 * work as expected
 */
TEST(Gossip, PeerSet) {
  srand(0);  // make test deterministic

  // 1. Create NT topic names

  const size_t NT = 7;
  std::array<g::TopicId, NT> all_topics;

  for (size_t i = 0; i < NT; ++i) {
    all_topics[i] = std::to_string(i + 1);
  }

  // 2. Create NP random peer contexts and subscribe them to
  // topics such as every M-th peer gets subscribed to M-th topic

  std::vector<g::PeerContextPtr> all_peers;

  const size_t NP = 100;
  all_peers.reserve(NP);
  for (size_t i = 0; i < NP; ++i) {
    auto pc = std::make_shared<g::PeerContext>(testutil::randomPeerId());
    for (size_t j = 0; j < NT; ++j) {
      if (i % (j + 1) == 0) {
        // every M-th peer gets subscribed to M-th topic
        pc->subscribed_to.insert(all_topics.at(j));
      }
    }
    all_peers.emplace_back(std::move(pc));
  }

  // 3. Insert peers into PeerSet

  g::PeerSet known_peers;
  for (const auto &pc : all_peers) {
    ASSERT_TRUE(known_peers.insert(pc));
  }

  // 4. Ensure all peers are in the set

  ASSERT_EQ(known_peers.size(), NP);
  for (size_t i = 0; i < NP; ++i) {
    auto pc = known_peers.find(all_peers[i]->peer_id);
    ASSERT_TRUE(pc);
  }

  // 5. Ensure that the set finds only what it contains

  ASSERT_FALSE(known_peers.find(g::getEmptyPeer()));

  // 6. Ensure the set selects 0 and 1 random peers

  std::vector<g::PeerContextPtr> vec;
  for (size_t i = 0; i <= 1; ++i) {
    vec = known_peers.selectRandomPeers(i);
    ASSERT_EQ(vec.size(), i);
  }

  // 7. Ensure the set selects N>1 random peers

  vec = known_peers.selectRandomPeers(NP / 2);
  ASSERT_EQ(vec.size(), NP / 2);
  for (const auto &selected_peer : vec) {
    auto pc = known_peers.find(selected_peer->peer_id);
    ASSERT_TRUE(pc);
    ASSERT_EQ(pc.value()->subscribed_to, selected_peer->subscribed_to);
    ASSERT_EQ(pc.value()->peer_id, selected_peer->peer_id);
  }

  // 8. Select only peers which are subscribed to topic #3 and ensure their
  // number meets subscription scheme above

  vec.clear();
  size_t selected_topic_no = 3;
  auto selected_topic = all_topics.at(selected_topic_no);
  known_peers.selectIf(
      [&vec](const g::PeerContextPtr &p) { vec.emplace_back(p); },

      [&](const g::PeerContextPtr &p) {
        return p->subscribed_to.count(selected_topic) != 0;
      });

  for (const auto &selected_peer : vec) {
    ASSERT_TRUE(selected_peer->subscribed_to.count(selected_topic) == 1);
  }
  ASSERT_EQ(vec.size(), NP / (selected_topic_no + 1));

  // 8. Erase peers which are subscribed to topic #4 and ensure that the whole
  // set has become smaller exactly as expected

  size_t deleted_topic_no = 4;
  auto deleted_topic = all_topics.at(deleted_topic_no);
  known_peers.eraseIf([&](const g::PeerContextPtr &p) {
    return p->subscribed_to.count(deleted_topic) != 0;
  });

  known_peers.selectAll([&](const g::PeerContextPtr &p) {
    ASSERT_TRUE(p->subscribed_to.count(deleted_topic) == 0);
  });
  ASSERT_EQ(known_peers.size(), NP - NP / (deleted_topic_no + 1));
}

/**
 * @given Empty MessageCache
 * @when We insert messages into it on different timestamp
 * @then We see that all messages are both inserted and expired properly
 */
TEST(Gossip, MessageCache) {
  constexpr g::Time msg_lifetime{20};
  constexpr g::Time timer_interval{msg_lifetime / 2};

  g::Time current_time{1234567890000ll};  // typically timestamp
  g::Time stop_time = current_time + g::Time{400};
  auto clock = [&current_time]() -> g::Time { return current_time; };

  // 1. Create the cache

  g::MessageCache cache(msg_lifetime, clock);

  // 2. Keep track of inserted messages

  uint64_t seq = 0;
  const auto fake_body = g::fromString("schnapps");
  std::vector<std::pair<g::Time, g::MessageId>> inserted_messages;

  // insert helper function
  auto insertMessage = [&](const g::TopicId &topic) {
    auto msg = std::make_shared<g::TopicMessage>(testutil::randomPeerId(),
                                                 seq++, fake_body, topic);
    auto msg_id = g::createMessageId(msg->from, msg->seq_no, msg->data);
    ASSERT_TRUE(cache.insert(msg, msg_id));
    inserted_messages.emplace_back(current_time, std::move(msg_id));
  };

  const g::TopicId topic_1("t1");
  const g::TopicId topic_2("t2");

  // 3. While increasing timestamp, insert messages into cache and
  // check their presence and expiration

  for (; current_time <= stop_time; ++current_time) {
    insertMessage(topic_1);
    if (current_time.count() % 2 == 1)
      insertMessage(topic_2);
    if (current_time.count() % timer_interval.count() == 0)
      cache.shift();
    if (current_time.count() % (timer_interval.count() * 10) == 0) {
      for (auto it = inserted_messages.rbegin(); it != inserted_messages.rend();
           ++it) {
        auto msg = cache.getMessage(it->second);
        if (it->first < current_time - msg_lifetime) {
          ASSERT_FALSE(msg);
          break;
        }
        ASSERT_TRUE(msg);
      }
    }
  }
}
