/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "src/protocol/gossip/impl/local_subscriptions.hpp"

#include <gtest/gtest.h>
#include <fmt/format.h>
#include "testutil/libp2p/peer.hpp"

// debug stuff, unpack if it's needed to debug and trace
#define VERBOSE 0

namespace g = libp2p::protocol::gossip;

namespace {

  // creates topic message for testing purposes
  g::TopicMessage::Ptr createTestMessage(g::TopicList topics, uint64_t seq) {
    std::string body = fmt::format("{}:{}", seq, fmt::join(topics, "-"));
    auto msg = std::make_shared<g::TopicMessage>(testutil::randomPeerId(), seq,
                                                 g::fromString(body));
    msg->topic_ids = std::move(topics);
    return msg;
  }

  std::shared_ptr<g::LocalSubscriptions> createSubscriptions(
      g::LocalSubscriptions::OnSubscriptionSetChange cb = {}) {
    if (!cb) {
      cb = [](bool subscribe, const g::TopicId &topic) {
        if (VERBOSE)
          fmt::print("{}{}\n", (subscribe ? "+" : "-"), topic);
      };
    }
    return std::make_shared<g::LocalSubscriptions>(std::move(cb));
  }

  // returns if topic containers intersect
  template <typename ContainerX, typename ContainerY>
  bool intersect(const ContainerX &x, const ContainerY &y) {
    for (const auto &topic : x) {
      if (y.count(topic) != 0) {
        return true;
      }
    }
    return false;
  }

  // per-subscription context
  struct SubscrCtx {
    std::set<g::ByteArray> received;
    size_t expected_count = 0;
    g::TopicSet topics;
    libp2p::protocol::Subscription subscr;

    void subscribe(g::LocalSubscriptions &subs, const g::TopicSet &ts) {
      topics = ts;

      subscr = subs.subscribe(ts, [this](g::Gossip::SubscriptionData d) {
        ASSERT_TRUE(d.has_value());

        // topic sets must intersect
        ASSERT_TRUE(intersect(d->topics, topics));

        // messages should not appear more than once
        ASSERT_EQ(received.count(d->data), 0);

        received.insert(d->data);
      });

      if (VERBOSE)
        fmt::print("subscribed to {}\n", fmt::join(topics, ","));
    }

    void unsubscribe() {
      subscr.cancel();
    }

    void checkExpected() {
      ASSERT_EQ(received.size(), expected_count);
    }
  };

}  // namespace

/**
 * @given LocalSubscriptions router
 * @when Creating a single subscription to a perdefined topic set, publishing
 * messages and unsubscribing in the middle of the process
 * @then Keeping track of messages received by subscription, we make sure
 * that all we were subscribed to is received without duplicates
 */
TEST(Gossip, OneSubscription) {
  auto subs = createSubscriptions();
  SubscrCtx ctx;

  ctx.subscribe(*subs, {"1", "2"});

  uint64_t seq = 0;

  for (size_t i = 0; i < 3; ++i) {
    subs->forwardMessage(createTestMessage({"1"}, seq++));
    subs->forwardMessage(createTestMessage({"1", "2"}, seq++));
    subs->forwardMessage(createTestMessage({"2"}, seq++));
    subs->forwardMessage(createTestMessage({"2", "3"}, seq++));
    subs->forwardMessage(createTestMessage({"3"}, seq++));
    if (i == 1) {
      // unsubscribing in the middle...
      ctx.unsubscribe();
    }
  }

  // 1) 15 == total messages sent
  // 2) unsubscribed after 10th message
  // 3) 4/5 of messages contain topics "1" or "2"
  // 4) then we expect 8 received messages
  ctx.expected_count = 8;
  ctx.checkExpected();
}

/**
 * @given LocalSubscriptions router
 * @when Creating 3 different subscriptions and publishing
 * messages
 * @then Keeping track of messages received by each subscription, we make sure
 * that all we were subscribed to is received without duplicates
 */
TEST(Gossip, MultipleSubscriptons) {
  auto subs = createSubscriptions();

  std::vector<std::unique_ptr<SubscrCtx>> ctx;
  g::TopicSet topics;
  for (size_t i = 1; i <= 3; ++i) {
    topics.insert(std::to_string(i));
    auto &s = ctx.emplace_back(std::make_unique<SubscrCtx>());
    s->subscribe(*subs, topics);
    s->expected_count = i;
  }

  uint64_t seq = 0;
  subs->forwardMessage(createTestMessage({"3"}, seq++));
  subs->forwardMessage(createTestMessage({"3", "2"}, seq++));
  subs->forwardMessage(createTestMessage({"3", "2", "1"}, seq++));
  subs->forwardMessage(createTestMessage({"xxx"}, seq));

  for (auto &s : ctx) {
    s->checkExpected();
  }
}
