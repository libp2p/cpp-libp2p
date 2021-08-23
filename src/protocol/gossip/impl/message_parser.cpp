/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_parser.hpp"

#include <libp2p/log/logger.hpp>

#include "message_receiver.hpp"

#include <generated/protocol/gossip/protobuf/rpc.pb.h>

namespace libp2p::protocol::gossip {

  namespace {
    auto log() {
      static auto logger = log::createLogger("gossip");
      return logger.get();
    }
  }  // namespace

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
    return pb_msg_->ParseFromArray(bytes.data(),
                                   static_cast<int>(bytes.size()));
  }

  void MessageParser::dispatch(const PeerContextPtr &from,
                               MessageReceiver &receiver) {
    if (!pb_msg_) {
      return;
    }

    for (const auto &s : pb_msg_->subscriptions()) {
      if (!s.has_subscribe() || !s.has_topicid()) {
        continue;
      }
      receiver.onSubscription(from, s.subscribe(), s.topicid());
    }

    if (pb_msg_->has_control()) {
      const auto &c = pb_msg_->control();

      for (const auto &h : c.ihave()) {
        if (!h.has_topicid() || h.messageids_size() == 0) {
          continue;
        }
        const TopicId &topic = h.topicid();
        for (const auto &msg_id : h.messageids()) {
          if (msg_id.empty()) {
            continue;
          }
          receiver.onIHave(from, topic, fromString(msg_id));
        }
      }

      for (const auto &w : c.iwant()) {
        if (w.messageids_size() == 0) {
          continue;
        }
        for (const auto &msg_id : w.messageids()) {
          if (msg_id.empty()) {
            continue;
          }
          receiver.onIWant(from, fromString(msg_id));
        }
      }

      for (const auto &gr : c.graft()) {
        if (!gr.has_topicid()) {
          continue;
        }
        receiver.onGraft(from, gr.topicid());
      }

      for (const auto &pr : c.prune()) {
        if (!pr.has_topicid()) {
          continue;
        }
        uint64_t backoff_time = 60;
        if (pr.has_backoff()) {
          backoff_time = pr.backoff();
        }
        log()->debug("prune backoff={}, {} peers", backoff_time,
                     pr.peers_size());
        for (const auto &peer : pr.peers()) {
          // TODO(artem): meshsub 1.1.0 + signed peer records NYI

          log()->debug("peer id size={}, signed peer record size={}",
                       peer.peerid().size(), peer.signedpeerrecord().size());
        }
        receiver.onPrune(from, pr.topicid(), backoff_time);
      }
    }

    for (const auto &m : pb_msg_->publish()) {
      if (!m.has_from() || !m.has_data() || !m.has_seqno() || !m.has_topic()) {
        continue;
      }
      auto message = std::make_shared<TopicMessage>(
          fromString(m.from()), fromString(m.seqno()), fromString(m.data()));
      message->topic = m.topic();
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
