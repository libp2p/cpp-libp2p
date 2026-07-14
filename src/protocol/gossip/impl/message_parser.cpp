/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_parser.hpp"

#include <libp2p/log/logger.hpp>

#include "message_receiver.hpp"

#include <generated/protocol/gossip/protobuf/rpc.pb.h>

#include "peer_context.hpp"

namespace libp2p::protocol::gossip {

  namespace {
    auto log() {
      static auto logger = log::createLogger("gossip");
      return logger.get();
    }
  }  // namespace

  // need to define default ctor/dtor here in translation unit due to unique_ptr
  // to type which is incomplete in header
  MessageParser::MessageParser(std::shared_ptr<RPCLimits> limits)
      : limits_(std::move(limits)) {}
  MessageParser::~MessageParser() = default;

  bool MessageParser::parse(BytesIn bytes) {
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

    size_t curr_subscriptions = 0;

    for (const auto &s : pb_msg_->subscriptions()) {
      if (!s.has_subscribe() || !s.has_topicid()) {
        continue;
      }

      if (curr_subscriptions == limits_->max_subscriptions) {
        break;
      }
      receiver.onSubscription(from, s.subscribe(), s.topicid());

      curr_subscriptions++;
    }

    if (pb_msg_->has_control()) {
      const auto &c = pb_msg_->control();
      size_t curr_ihave_messages = 0;
      size_t curr_iwant_messages = 0;
      size_t curr_graft_messages = 0;
      size_t curr_prune_messages = 0;

      for (const auto &h : c.ihave()) {
        if (curr_ihave_messages == limits_->max_ihave_messages) {
          break;
        }
        size_t curr_ihave_message_ids = 0;
        if (!h.has_topicid() || h.messageids_size() == 0) {
          continue;
        }
        const TopicId &topic = h.topicid();
        for (const auto &msg_id : h.messageids()) {
          if (curr_ihave_message_ids == limits_->max_ihave_message_ids) {
            break;
          }
          if (msg_id.empty()) {
            continue;
          }
          receiver.onIHave(from, topic, fromString(msg_id));

          curr_ihave_message_ids++;
        }

        curr_ihave_messages++;
      }

      for (const auto &w : c.iwant()) {
        if (curr_iwant_messages == limits_->max_iwant_messages) {
          break;
        }
        size_t curr_iwant_message_ids = 0;
        if (w.messageids_size() == 0) {
          continue;
        }
        for (const auto &msg_id : w.messageids()) {
          if (curr_iwant_message_ids == limits_->max_iwant_message_ids) {
            break;
          }
          if (msg_id.empty()) {
            continue;
          }
          receiver.onIWant(from, fromString(msg_id));

          curr_iwant_message_ids++;
        }
        curr_iwant_messages++;
      }

      for (const auto &gr : c.graft()) {
        if (curr_graft_messages == limits_->max_graft_messages) {
          break;
        }
        if (!gr.has_topicid()) {
          continue;
        }
        receiver.onGraft(from, gr.topicid());

        curr_graft_messages++;
      }

      for (const auto &pr : c.prune()) {
        if (curr_prune_messages == limits_->max_prune_messages) {
          break;
        }
        size_t curr_prune_peer_infos = 0;
        if (!pr.has_topicid()) {
          continue;
        }
        uint64_t backoff_time = 60;
        if (pr.has_backoff()) {
          backoff_time = pr.backoff();
        }
        log()->debug(
            "prune backoff={}, {} peers", backoff_time, pr.peers_size());
        for (const auto &peer : pr.peers()) {
          if (curr_prune_peer_infos == limits_->max_prune_peer_infos) {
            break;
          }
          // TODO(artem): meshsub 1.1.0 + signed peer records NYI

          log()->debug("peer id size={}, signed peer record size={}",
                       peer.peerid().size(),
                       peer.signedpeerrecord().size());

          curr_prune_peer_infos++;
        }
        receiver.onPrune(from, pr.topicid(), backoff_time);

        curr_prune_messages++;
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
