/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_builder.hpp"

#include <libp2p/multi/uvarint.hpp>

#include <generated/protocol/gossip/protobuf/rpc.pb.h>

namespace libp2p::protocol::gossip {

  namespace {
    // helper needed since protobuf doesn't have a blob type
    inline const char *toString(const ByteArray &bytes) {
      // NOLINTNEXTLINE
      return reinterpret_cast<const char *>(bytes.data());
    }
  }  // namespace

  MessageBuilder::MessageBuilder() : empty_(true), control_not_empty_(false) {}

  MessageBuilder::~MessageBuilder() = default;

  void MessageBuilder::clear() {
    pb_msg_->Clear();
    control_pb_msg_->Clear();
    empty_ = true;
    control_not_empty_ = false;
    ihaves_.clear();
    iwant_.clear();
    messages_added_.clear();
  }

  void MessageBuilder::reset() {
    pb_msg_.reset();
    control_pb_msg_.reset();
    empty_ = true;
    control_not_empty_ = false;
    decltype(ihaves_){}.swap(ihaves_);
    decltype(iwant_){}.swap(iwant_);
    decltype(messages_added_){}.swap(messages_added_);
  }

  void MessageBuilder::create_protobuf_structures() {
    if (pb_msg_ == nullptr) {
      pb_msg_ = std::make_unique<pubsub::pb::RPC>();
      control_pb_msg_ = std::make_unique<pubsub::pb::ControlMessage>();
    }
  }

  bool MessageBuilder::empty() const {
    return empty_;
  }

  outcome::result<SharedBuffer> MessageBuilder::serialize() {
    create_protobuf_structures();

    for (auto &[topic, message_ids] : ihaves_) {
      auto *ih = control_pb_msg_->add_ihave();
      ih->set_topicid(topic);
      for (auto &mid : message_ids) {
        ih->add_messageids(toString(mid), mid.size());
      }
    }

    if (!iwant_.empty()) {
      auto *iw = control_pb_msg_->add_iwant();
      for (auto &mid : iwant_) {
        iw->add_messageids(toString(mid), mid.size());
      }
    }

    if (control_not_empty_) {
      pb_msg_->set_allocated_control(control_pb_msg_.get());
    }

    size_t msg_sz = pb_msg_->ByteSizeLong();

    auto varint_len = multi::UVarint{msg_sz};
    auto varint_vec = varint_len.toVector();
    size_t prefix_sz = varint_vec.size();

    auto buffer = std::make_shared<ByteArray>();
    buffer->resize(prefix_sz + msg_sz);
    memcpy(buffer->data(), varint_vec.data(), prefix_sz);

    bool success =
        // NOLINTNEXTLINE
        pb_msg_->SerializeToArray(buffer->data() + prefix_sz, msg_sz);

    if (control_not_empty_) {
      pb_msg_->release_control();
    }

    static constexpr size_t kSizeThreshold = 8192;
    if (msg_sz > kSizeThreshold) {
      reset();
    } else {
      clear();
    }

    if (success) {
      return buffer;
    }
    return outcome::failure(Error::MESSAGE_SERIALIZE_ERROR);
  }

  void MessageBuilder::addSubscription(bool subscribe, const TopicId &topic) {
    create_protobuf_structures();

    auto *dst = pb_msg_->add_subscriptions();
    dst->set_subscribe(subscribe);
    dst->set_topicid(topic);
    empty_ = false;
  }

  void MessageBuilder::addIHave(const TopicId &topic, const MessageId &msg_id) {
    ihaves_[topic].push_back(msg_id);
    control_not_empty_ = true;
    empty_ = false;
  }

  void MessageBuilder::addIWant(const MessageId &msg_id) {
    iwant_.push_back(msg_id);
    control_not_empty_ = true;
    empty_ = false;
  }

  void MessageBuilder::addGraft(const TopicId &topic) {
    create_protobuf_structures();

    control_pb_msg_->add_graft()->set_topicid(topic);
    control_not_empty_ = true;
    empty_ = false;
  }

  void MessageBuilder::addPrune(const TopicId &topic) {
    create_protobuf_structures();

    control_pb_msg_->add_prune()->set_topicid(topic);
    control_not_empty_ = true;
    empty_ = false;
  }

  void MessageBuilder::addMessage(const TopicMessage &msg,
                                  const MessageId &msg_id) {
    create_protobuf_structures();

    if (messages_added_.count(msg_id) != 0) {
      // prevent duplicates
      return;
    }
    messages_added_.insert(msg_id);

    auto *dst = pb_msg_->add_publish();
    dst->set_from(msg.from.data(), msg.from.size());
    dst->set_data(msg.data.data(), msg.data.size());
    dst->set_seqno(msg.seq_no.data(), msg.seq_no.size());
    dst->set_topic(msg.topic);
    if (msg.signature) {
      dst->set_signature(msg.signature.value().data(),
                         msg.signature.value().size());
    }
    if (msg.key) {
      dst->set_key(msg.key.value().data(), msg.key.value().size());
    }
    empty_ = false;
  }

  outcome::result<ByteArray> MessageBuilder::signableMessage(
      const TopicMessage &msg) {
    pubsub::pb::Message pb_msg;
    pb_msg.set_from(msg.from.data(), msg.from.size());
    pb_msg.set_data(msg.data.data(), msg.data.size());
    pb_msg.set_seqno(msg.seq_no.data(), msg.seq_no.size());
    pb_msg.set_topic(msg.topic);
    constexpr std::string_view kPrefix{"libp2p-pubsub:"};
    auto size = pb_msg.ByteSizeLong();
    ByteArray signable;
    signable.resize(kPrefix.size() + size);
    std::copy(kPrefix.begin(), kPrefix.end(), signable.begin());
    if (!pb_msg.SerializeToArray(&signable[kPrefix.size()],
                                 static_cast<int>(size))) {
      return outcome::failure(Error::MESSAGE_SERIALIZE_ERROR);
    }
    return signable;
  }
}  // namespace libp2p::protocol::gossip
