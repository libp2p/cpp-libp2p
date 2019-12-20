/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/gossip_wire_protocol.hpp>
#include <libp2p/protocol/gossip/impl/message_cache.hpp>
#include <libp2p/protocol/gossip/impl/peers.hpp>

#include <gtest/gtest.h>
#include <spdlog/fmt/fmt.h>
#include "testutil/libp2p/peer.hpp"

static bool VERBOSE = false;

using fmt::print;

#define TR(var) \
  if (VERBOSE)  \
  fmt::print("{}: {}={}\n", __LINE__, #var, var)

namespace g = libp2p::protocol::gossip;

TEST(Gossip, CommonStuff) {
  auto peer = testutil::randomPeerId();
  auto msg = g::TopicMessage::fromScratch(peer, 0x2233445566778899ull,
                                          g::fromString("hahaha"));
  auto peer_res = peerFrom(*msg);
  ASSERT_TRUE(peer_res);
  ASSERT_EQ(peer, peer_res.value());
  ASSERT_EQ(msg->seq_no,
            g::ByteArray({0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}));
  g::MessageId id = g::createMessageId(*msg);
  ASSERT_EQ(id.size(), 42);
}

TEST(Gossip, PeerSet) {
  srand(0);  // make test deterministic

  const size_t NT = 7;
  std::array<g::TopicId, NT> all_topics;
  for (size_t i = 0; i < NT; ++i) {
    all_topics[i] = std::to_string(i + 1);
  }

  std::vector<g::PeerContext::Ptr> all_peers;
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

  g::PeerSet known_peers;
  for (const auto &pc : all_peers) {
    ASSERT_TRUE(known_peers.insert(pc));
  }

  for (size_t i = 0; i < NP; ++i) {
    auto pc = known_peers.find(all_peers[i]->peer_id);
    ASSERT_TRUE(pc);
  }

  ASSERT_FALSE(known_peers.find(g::getEmptyPeer()));

  std::vector<g::PeerContext::Ptr> vec;
  for (size_t i = 0; i < 3; ++i) {
    vec = known_peers.selectRandomPeers(i);
    ASSERT_EQ(vec.size(), i);
  }

  vec = known_peers.selectRandomPeers(NP / 2);
  ASSERT_EQ(vec.size(), NP / 2);
  for (const auto &selected_peer : vec) {
    auto pc = known_peers.find(selected_peer->peer_id);
    ASSERT_TRUE(pc);
    ASSERT_EQ(pc.value()->subscribed_to, selected_peer->subscribed_to);
    ASSERT_EQ(pc.value()->peer_id, selected_peer->peer_id);
  }

  vec.clear();
  size_t selected_topic_no = 3;
  auto selected_topic = all_topics.at(selected_topic_no);
  known_peers.selectIf(
      [&vec](const g::PeerContext::Ptr &p) { vec.emplace_back(p); },

      [&](const g::PeerContext::Ptr &p) {
        return p->subscribed_to.count(selected_topic) != 0;
      });

  for (const auto &selected_peer : vec) {
    ASSERT_TRUE(selected_peer->subscribed_to.count(selected_topic) == 1);
  }

  ASSERT_EQ(vec.size(), NP / (selected_topic_no + 1));

  size_t deleted_topic_no = 4;
  auto deleted_topic = all_topics.at(deleted_topic_no);
  known_peers.eraseIf([&](const g::PeerContext::Ptr &p) {
    return p->subscribed_to.count(deleted_topic) != 0;
  });

  known_peers.selectAll([&](const g::PeerContext::Ptr &p) {
    ASSERT_TRUE(p->subscribed_to.count(deleted_topic) == 0);
  });

  ASSERT_EQ(known_peers.size(), NP - NP / (deleted_topic_no + 1));
}

TEST(Gossip, MessageCache) {
  constexpr g::Time msg_lifetime = 20;
  constexpr g::Time broadcast_lifetime = 10;
  constexpr g::Time timer_interval = broadcast_lifetime;

  g::Time current_time = 1234567890000ull;  // typically timestamp
  g::Time stop_time = current_time + 40;
  auto clock = [&current_time]() -> g::Time { return current_time; };

  g::MessageCache cache(msg_lifetime, broadcast_lifetime, clock);

  uint64_t seq = 0;
  const auto fake_body = g::fromString("schnapps");
  std::vector<std::pair<g::Time, g::MessageId>> inserted_messages;

  auto insertMessage = [&](const g::TopicId& topic) {
    auto msg = g::TopicMessage::fromScratch(
        testutil::randomPeerId(), seq++, fake_body);
    msg->topic_ids.push_back(topic);
    auto id_res = cache.insert(std::move(msg));
    ASSERT_TRUE(id_res);
    inserted_messages.emplace_back( current_time, std::move(id_res.value()) );
  };

  const g::TopicId topic_1("t1");
  const g::TopicId topic_2("t2");

  for (; current_time <= stop_time; ++current_time) {
    insertMessage(topic_1);
    if (current_time % 2 == 1) insertMessage(topic_2);
    if (current_time % timer_interval == 0) cache.shift();
    if (current_time % (timer_interval * 10) == 0) {
      for (auto it = inserted_messages.rbegin();
                it != inserted_messages.rend(); ++it) {
        auto msg = cache.getMessage(it->second);
        if (it->first < current_time - msg_lifetime) {
          ASSERT_FALSE(msg);
          break;
        }
        ASSERT_TRUE(msg);
      }
    }
  }

  size_t expected_topic_1 = broadcast_lifetime;
  size_t expected_topic_2 = broadcast_lifetime / 2;
  size_t seen_topic_1 = 0;
  size_t seen_topic_2 = 0;

  auto seenCallback = [&](const g::TopicId& topic, const g::MessageId&) {
    if (topic == topic_1) ++seen_topic_1;
    else if (topic == topic_2) ++seen_topic_2;
    else {
      TR(topic);
      ASSERT_EQ("unknown topic", nullptr);
    }
  };

  cache.getSeenMessageIds("", seenCallback);
  ASSERT_EQ(expected_topic_2, seen_topic_2);
  ASSERT_EQ(expected_topic_1, seen_topic_1);

}
