/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_builder.hpp"

#include <libp2p/multi/uvarint.hpp>
#include <qtils/bytestr.hpp>

#include <generated/protocol/gossip/protobuf/rpc.pb.h>

namespace libp2p::protocol::gossip {

  namespace {
    // helper needed since protobuf doesn't have a blob type
    inline const char *toString(const Bytes &bytes) {
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
    subscriptions_.clear();
    idontwant_.clear();
    ihaves_.clear();
    iwant_.clear();
    messages_added_.clear();
  }

  void MessageBuilder::reset() {
    pb_msg_.reset();
    control_pb_msg_.reset();
    empty_ = true;
    control_not_empty_ = false;
    std::exchange(subscriptions_, {});
    std::exchange(idontwant_, {});
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

    for (auto &[topic, subscribe] : subscriptions_) {
      auto *subscription = pb_msg_->add_subscriptions();
      subscription->set_subscribe(subscribe);
      subscription->set_topicid(topic);
    }

    if (not idontwant_.empty()) {
      auto *idontwant = control_pb_msg_->add_idontwant();
      for (auto &msg_id : idontwant_) {
        *idontwant->add_message_ids() = qtils::byte2str(msg_id);
      }
    }

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

    auto buffer = std::make_shared<Bytes>();
    buffer->resize(prefix_sz + msg_sz);
    memcpy(buffer->data(), varint_vec.data(), prefix_sz);

    bool success =
        // NOLINTNEXTLINE
        pb_msg_->SerializeToArray(buffer->data() + prefix_sz, msg_sz);

    if (control_not_empty_) {
      std::ignore = pb_msg_->release_control();
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
    return Error::MESSAGE_SERIALIZE_ERROR;
  }

  void MessageBuilder::addSubscription(bool subscribe, const TopicId &topic) {
    auto it = subscriptions_.emplace(topic, subscribe).first;
    if (it->second != subscribe) {
      subscriptions_.erase(it);
    }
    empty_ = false;
  }

  void MessageBuilder::addIDontWant(const MessageId &msg_id) {
    idontwant_.emplace(msg_id);
    control_not_empty_ = true;
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

  void MessageBuilder::addPrune(const TopicId &topic,
                                std::optional<std::chrono::seconds> backoff) {
    create_protobuf_structures();

    auto *prune = control_pb_msg_->add_prune();
    prune->set_topicid(topic);
    if (backoff.has_value()) {
      prune->set_backoff(backoff->count());
    }
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

    toPb(*pb_msg_->add_publish(), msg);
    empty_ = false;
  }

  outcome::result<Bytes> MessageBuilder::signableMessage(
      const TopicMessage &msg) {
    pubsub::pb::Message pb_msg;
    pb_msg.set_from(msg.from.data(), msg.from.size());
    pb_msg.set_data(msg.data.data(), msg.data.size());
    pb_msg.set_seqno(msg.seq_no.data(), msg.seq_no.size());
    pb_msg.set_topic(msg.topic);
    constexpr std::string_view kPrefix{"libp2p-pubsub:"};
    auto size = pb_msg.ByteSizeLong();
    Bytes signable;
    signable.resize(kPrefix.size() + size);
    std::copy(kPrefix.begin(), kPrefix.end(), signable.begin());
    if (!pb_msg.SerializeToArray(&signable[kPrefix.size()],
                                 static_cast<int>(size))) {
      return Error::MESSAGE_SERIALIZE_ERROR;
    }
    return signable;
  }

  void MessageBuilder::toPb(pubsub::pb::Message &pb_message,
                            const TopicMessage &message) {
    pb_message.set_from(message.from.data(), message.from.size());
    pb_message.set_data(message.data.data(), message.data.size());
    pb_message.set_seqno(message.seq_no.data(), message.seq_no.size());
    pb_message.set_topic(message.topic);
    if (message.signature) {
      pb_message.set_signature(message.signature.value().data(),
                               message.signature.value().size());
    }
    if (message.key) {
      pb_message.set_key(message.key.value().data(),
                         message.key.value().size());
    }
  }

  size_t MessageBuilder::pbSize(const TopicMessage &message) {
    static thread_local pubsub::pb::Message pb_message;
    toPb(pb_message, message);
    return pb_message.ByteSizeLong();
  }
}  // namespace libp2p::protocol::gossip
