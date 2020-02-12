/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/message_parser.hpp>

#include <libp2p/protocol/gossip/impl/message_receiver.hpp>

#include <generated/protocol/gossip/protobuf/rpc.pb.h>

namespace libp2p::protocol::gossip {

  // need to define default ctor/dtor here in translation unit due to unique_ptr
  // to type which is incomplete in header
  MessageParser::MessageParser() = default;
  MessageParser::~MessageParser() = default;

  bool MessageParser::parse(gsl::span<const uint8_t> bytes) {
    if (!pb_msg_) {
      pb_msg_ = std::make_unique<pubsub::pb::RPC>();
    } else {
      pb_msg_->Clear();
    }
    return pb_msg_->ParseFromArray(bytes.data(), bytes.size());
  }

  void MessageParser::dispatch(const PeerContextPtr &from,
                               MessageReceiver &receiver) {
    if (!pb_msg_) {
      return;
    }

    for (auto &s : pb_msg_->subscriptions()) {
      if (!s.has_subscribe() || !s.has_topicid()) {
        continue;
      }
      receiver.onSubscription(from, s.subscribe(), s.topicid());
    }

    if (pb_msg_->has_control()) {
      auto &c = pb_msg_->control();

      for (auto &h : c.ihave()) {
        if (!h.has_topicid() || h.messageids_size() == 0) {
          continue;
        }
        const TopicId &topic = h.topicid();
        for (auto &msg_id : h.messageids()) {
          if (msg_id.empty()) {
            continue;
          }
          receiver.onIHave(from, topic, fromString(msg_id));
        }
      }

      for (auto &w : c.iwant()) {
        if (w.messageids_size() == 0) {
          continue;
        }
        for (auto &msg_id : w.messageids()) {
          if (msg_id.empty()) {
            continue;
          }
          receiver.onIWant(from, fromString(msg_id));
        }
      }

      for (auto &gr : c.graft()) {
        if (!gr.has_topicid()) {
          continue;
        }
        receiver.onGraft(from, gr.topicid());
      }

      for (auto &pr : c.prune()) {
        if (!pr.has_topicid()) {
          continue;
        }
        receiver.onPrune(from, pr.topicid());
      }
    }

    for (auto &m : pb_msg_->publish()) {
      if (!m.has_from() || !m.has_data() || !m.has_seqno()
          || m.topicids_size() == 0) {
        continue;
      }
      auto message = std::make_shared<TopicMessage>(
          fromString(m.from()), fromString(m.seqno()), fromString(m.data()));
      for (auto &tid : m.topicids()) {
        message->topic_ids.push_back(tid);
      }
      if (m.has_signature()) {
        message->signature = fromString(m.signature());
      }
      if (m.has_key()) {
        message->key = fromString(m.key());
      }
      receiver.onTopicMessage(from, std::move(message));
    }

    receiver.onMessageEnd(from);
  }

}  // namespace libp2p::protocol::gossip
