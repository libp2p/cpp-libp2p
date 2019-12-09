/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gossip_wire_protocol.hpp"

#include <libp2p/multi/uvarint.hpp>

#include <generated/protocol/gossip/protobuf/rpc.pb.h>

namespace libp2p::protocol::gossip {

  namespace {

    inline Bytes fromString(const std::string& s) {
      Bytes ret;
      auto sz = s.size();
      if (sz > 0) {
        ret.reserve(sz);
        ret.assign(s.begin(), s.end());
      }
      return ret;
    }

  } //namespace

  bool GossipMessage::empty() const {
    return subscriptions.empty() && publish.empty()
      && (!control || control->empty());
  }

  void GossipMessage::clear() {
    subscriptions.clear();
    publish.clear();
    if (control) control->clear();
  }

  bool ControlMessage::empty() const {
    return i_have.empty() && i_want.empty() && graft.empty() && prune.empty();
  }

  void ControlMessage::clear() {
    i_have.clear();
    i_want.clear();
    graft.clear();
    prune.clear();
  }

  bool GossipMessage::deserialize(gsl::span<const uint8_t> bytes) {
    clear();

    pubsub::pb::RPC pb_msg;
    if (!pb_msg.ParseFromArray(bytes.data(), bytes.size())) {
      return false;
    }

    for (auto& s : pb_msg.subscriptions()) {
      if (!s.has_subscribe() || !s.has_topicid()) {
        continue;
      }
      SubOpts& dst = subscriptions.emplace_back();
      dst.subscribe = s.subscribe();
      dst.topic_id = s.topicid();
    }

    for (auto& m : pb_msg.publish()) {
      if (!m.has_from()
        || !m.has_data()
        || !m.has_seqno()
        || m.topicids_size() == 0) {
        continue;
      }
      TopicMessage& dst = publish.emplace_back();
      dst.from = fromString(m.from());
      dst.data = fromString(m.data());
      dst.seq_no = fromString(m.seqno());
      for (auto& tid : m.topicids()) {
        dst.topic_ids.push_back(tid);
      }
      if (m.has_signature()) {
        dst.signature = fromString(m.signature());
      }
      if (m.has_key()) {
        dst.key = fromString(m.key());
      }
    }

    if (pb_msg.has_control()) {
      control = ControlMessage{};
      auto& src = pb_msg.control();

      for (auto& h : src.ihave()) {
        if (!h.has_topicid() || h.messageids_size() == 0) {
          continue;
        }
        auto& dst = control->i_have.emplace_back();
        dst.topic_id = h.topicid();
        dst.message_ids.reserve(h.messageids_size());
        for (auto& msgid : h.messageids()) {
          dst.message_ids.push_back(msgid);
        }
      }

      for (auto& w : src.iwant()) {
        if (w.messageids_size() == 0) {
          continue;
        }
        for (auto& msgid : w.messageids()) {
          control->i_want.push_back(msgid);
        }
      }

      control->graft.reserve(src.graft_size());
      for (auto& gr : src.graft()) {
        if (!gr.has_topicid()) {
          continue;
        }
        control->graft.push_back(gr.topicid());
      }

      control->prune.reserve(src.prune_size());
      for (auto& pr : src.prune()) {
        if (!pr.has_topicid()) {
          continue;
        }
        control->graft.push_back(pr.topicid());
      }
    }

    return !empty();
  }

  bool GossipMessage::serialize(std::vector<uint8_t>& buffer) const {
    pubsub::pb::RPC pb_msg;

    for (auto& sub : subscriptions) {
      auto* dst = pb_msg.add_subscriptions();
      dst->set_subscribe(sub.subscribe);
      dst->set_topicid(sub.topic_id);
    }

    for (auto& pub : publish) {
      auto* dst = pb_msg.add_publish();
      dst->set_from(pub.from.data(), pub.from.size());
      dst->set_data(pub.data.data(), pub.data.size());
      dst->set_seqno(pub.seq_no.data(), pub.seq_no.size());
      for (auto& id : pub.topic_ids) {
        *dst->add_topicids() = id;
      }
      if (pub.signature) {
        dst->set_signature(pub.signature.value().data(),
                           pub.signature.value().size());
      }
      if (pub.key) {
        dst->set_key(pub.key.value().data(),
                     pub.key.value().size());
      }
    }

    pubsub::pb::ControlMessage* pcmsg = nullptr;
    pubsub::pb::ControlMessage pb_cmsg;

    if (control) {
      const auto& c = control.value();

      for (auto& ihave : c.i_have) {
        auto* h = pb_cmsg.add_ihave();
        h->set_topicid(ihave.topic_id);
        for (auto& mid : ihave.message_ids) {
          h->add_messageids(mid);
        }
      }

      if (!c.i_want.empty()) {
        auto* w = pb_cmsg.add_iwant();
        for (auto& mid : c.i_want) {
          w->add_messageids(mid);
        }
      }

      for (auto& gr : c.graft) {
        pb_cmsg.add_graft()->set_topicid(gr);
      }

      for (auto& pr : c.prune) {
        pb_cmsg.add_prune()->set_topicid(pr);
      }

      pcmsg = &pb_cmsg;
      pb_msg.set_allocated_control(pcmsg);
    }

    size_t msg_sz = pb_msg.ByteSizeLong();

    auto varint_len = multi::UVarint{msg_sz};
    auto varint_vec = varint_len.toVector();
    size_t prefix_sz = varint_vec.size();

    buffer.resize(prefix_sz + msg_sz);
    memcpy(buffer.data(), varint_vec.data(), prefix_sz);

    bool ret = pb_msg.SerializeToArray(
        buffer.data() + prefix_sz, //NOLINT
        msg_sz);

    if (pcmsg != nullptr) {
      pb_msg.release_control();
    }

    return ret;
  }

} //namespace libp2p::protocol::gossip
