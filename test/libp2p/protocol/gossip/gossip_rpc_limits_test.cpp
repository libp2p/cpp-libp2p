/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocol/gossip/protobuf/rpc.pb.h"
#include "src/protocol/gossip/impl/common.hpp"
#include "src/protocol/gossip/impl/message_parser.hpp"
#include "src/protocol/gossip/impl/message_receiver.hpp"
#include "src/protocol/gossip/impl/peer_context.hpp"
#include "testutil/libp2p/peer.hpp"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>
#include <map>
#include <memory>
#include <set>
#include <soralog/configurator.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/level.hpp>
#include <soralog/logging_system.hpp>
#include <string>

namespace g = libp2p::protocol::gossip;

namespace {
  class TestMessageReceiver : public g::MessageReceiver {
   public:
    ~TestMessageReceiver() = default;

    void onSubscription(const g::PeerContextPtr &from,
                        bool subscribe,
                        const g::TopicId &topic) {
      subscriptions_processed++;
    }

    void onIHave(const g::PeerContextPtr &from,
                 const g::TopicId &topic,
                 const g::MessageId &msg_id) {
      if (!ihave_topicIds_processed.contains(topic)) {
        ihave_topicIds_processed.insert(topic);
        ihave_messages_processed++;
      }
      ihave_message_ids_processed[topic]++;
    }

    void onIWant(const g::PeerContextPtr &from, const g::MessageId &msg_id) {
      total_iwant_message_Ids_processed++;
    }

    void onGraft(const g::PeerContextPtr &from, const g::TopicId &topic) {
      graft_processed++;
    }

    void onPrune(const g::PeerContextPtr &from,
                 const g::TopicId &topic,
                 uint64_t backoff_time) {
      prune_processed++;
    }

    void onTopicMessage(const g::PeerContextPtr &from,
                        g::TopicMessage::Ptr msg) {}

    void onMessageEnd(const g::PeerContextPtr &from) {}

    size_t subscriptions_processed = 0;

    /// IHAVE Control messages can have multiple message ids and each control
    /// message group can be uniquely identified using a topic Id
    size_t ihave_messages_processed = 0;
    std::set<g::TopicId> ihave_topicIds_processed{};
    std::map<g::TopicId, size_t> ihave_message_ids_processed{};

    /// IWANT Control messages can have multiple message ids but each control
    /// message group cannot be uniquely identified
    size_t total_iwant_message_Ids_processed = 0;

    size_t graft_processed = 0;

    /// NOTE: Add Peer Info count after MessageParser Update
    /// PRUNE messages can have multiple peer infos, but it's currently not
    /// implemented in the parser.
    size_t prune_processed = 0;
  };

  void serializeAndDispatch(pubsub::pb::RPC &rpc,
                            g::MessageParser &parser,
                            TestMessageReceiver &receiver) {
    std::string serialized;
    rpc.SerializeToString(&serialized);
    libp2p::BytesIn pubsub_message{
        reinterpret_cast<const uint8_t *>(serialized.data()),
        serialized.size()};
    parser.parse(pubsub_message);
    const g::PeerContextPtr mock_context_ptr =
        std::make_shared<g::PeerContext>(testutil::randomPeerId());
    parser.dispatch(mock_context_ptr, receiver);
  }
}  // namespace

/**
 * @given Limit on subscriptions in RPC
 * @when we parse the RPC message
 * @then subscriptions after the limit has been reached are ignored
 */
TEST(Gossip, RPCSubscriptionLimit) {
  srand(0);
  g::RPCLimits limits{};
  limits.max_subscriptions = rand() % 10;

  pubsub::pb::RPC rpc;
  for (int i = 0; i < 100; i++) {
    auto *sub = rpc.add_subscriptions();
    sub->set_subscribe(i % 2);
    sub->set_topicid(std::to_string(i));
  }
  g::MessageParser parser{std::make_shared<g::RPCLimits>(limits)};
  TestMessageReceiver receiver;
  serializeAndDispatch(rpc, parser, receiver);
  ASSERT_EQ(receiver.subscriptions_processed, limits.max_subscriptions);
}

/**
 * @given Limit on IHAVE control messages and IHAVE message ids on RPC
 * @when we parse the RPC message
 * @then messages and message ids after the limit are ignored
 */
TEST(Gossip, RPCIHaveLimit) {
  srand(0);
  g::RPCLimits limits{};
  limits.max_ihave_messages = rand() % 10;
  limits.max_ihave_message_ids = rand() % 10;

  pubsub::pb::RPC rpc;
  auto *control = rpc.mutable_control();
  for (int i = 0; i < 100; i++) {
    auto *ihave = control->add_ihave();
    ihave->set_topicid(std::to_string(i));
    for (int j = 0; j < 100; j++) {
      ihave->add_messageids(std::to_string(j));
    }
  }

  g::MessageParser parser{std::make_shared<g::RPCLimits>(limits)};
  TestMessageReceiver receiver;
  serializeAndDispatch(rpc, parser, receiver);
  ASSERT_EQ(receiver.ihave_messages_processed, limits.max_ihave_messages);
  for (size_t i = 0; i < limits.max_ihave_messages; i++) {
    ASSERT_EQ(receiver.ihave_message_ids_processed[std::to_string(i)],
              limits.max_ihave_message_ids);
  }
  for (int i = limits.max_ihave_messages; i < 100; i++) {
    ASSERT_EQ(receiver.ihave_message_ids_processed[std::to_string(i)], 0);
  }
}

/**
 * @given Limit on IWANT control messages and IWANT message ids on RPC
 * @when we parse the RPC message
 * @then messages and message ids after the limit are ignored
 */
TEST(Gossip, RPCIWantLimit) {
  srand(0);
  g::RPCLimits limits{};
  limits.max_iwant_messages = rand() % 10;
  limits.max_iwant_message_ids = rand() % 10;

  pubsub::pb::RPC rpc;
  auto *control = rpc.mutable_control();
  for (int i = 0; i < 100; i++) {
    auto *iwant = control->add_iwant();
    for (int j = 0; j < 100; j++) {
      iwant->add_messageids(std::to_string(j));
    }
  }

  g::MessageParser parser{std::make_shared<g::RPCLimits>(limits)};
  TestMessageReceiver receiver;
  serializeAndDispatch(rpc, parser, receiver);
  ASSERT_EQ(receiver.total_iwant_message_Ids_processed,
            limits.max_iwant_messages * limits.max_iwant_message_ids);
}

/**
 * @given Limit on GRAFT control messages on RPC
 * @when we parse the RPC message
 * @then messages after the limit are ignored
 */
TEST(Gossip, RPCGraftLimit) {
  srand(0);
  g::RPCLimits limits{};
  limits.max_graft_messages = rand() % 10;

  pubsub::pb::RPC rpc;
  auto *control = rpc.mutable_control();
  for (int i = 0; i < 100; i++) {
    auto *graft = control->add_graft();
    graft->set_topicid(std::to_string(i));
  }

  g::MessageParser parser{std::make_shared<g::RPCLimits>(limits)};
  TestMessageReceiver receiver;
  serializeAndDispatch(rpc, parser, receiver);

  ASSERT_EQ(receiver.graft_processed, limits.max_graft_messages);
}

/**
 * @given Limit on PRUNE control messages on RPC
 * @when we parse the RPC message
 * @then messages after the limit are ignored
 */
TEST(Gossip, RPCPruneLimit) {
  // Logging system is required as PRUNE control message parsing logs
  const std::string logger_config(R"(
# ----------------
sinks:
  - name: console
    type: console
    color: true
groups:
  - name: main
    sink: console
    level: debug
    children:
      - name: libp2p
# ----------------
  )");

  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<soralog::ConfiguratorFromYAML>(
          // Original LibP2P logging config
          std::make_shared<libp2p::log::Configurator>(),
          // Additional logging config for application
          logger_config));
  auto result = logging_system->configure();
  libp2p::log::setLoggingSystem(logging_system);

  srand(0);
  g::RPCLimits limits{};
  limits.max_prune_messages = rand() % 10;

  pubsub::pb::RPC rpc;
  auto *control = rpc.mutable_control();
  for (int i = 0; i < 100; i++) {
    auto *prune = control->add_prune();
    prune->set_topicid(std::to_string(i));
  }

  g::MessageParser parser{std::make_shared<g::RPCLimits>(limits)};
  TestMessageReceiver receiver;
  serializeAndDispatch(rpc, parser, receiver);

  ASSERT_EQ(receiver.prune_processed, limits.max_prune_messages);
}
