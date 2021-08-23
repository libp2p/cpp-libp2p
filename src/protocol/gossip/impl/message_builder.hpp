/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_MESSAGE_BUILDER_HPP
#define LIBP2P_PROTOCOL_GOSSIP_MESSAGE_BUILDER_HPP

#include <map>
#include <unordered_set>

#include "common.hpp"

namespace pubsub::pb {
  // protobuf entities forward declaration
  class RPC;
  class ControlMessage;
}  // namespace pubsub::pb

namespace libp2p::protocol::gossip {

  /// Constructs RPC message as new fields added and serializes it
  /// into bytes before sending into wire
  class MessageBuilder {
   public:
    MessageBuilder(MessageBuilder &&) = default;
    MessageBuilder &operator=(MessageBuilder &&) = default;
    MessageBuilder(const MessageBuilder &) = delete;
    MessageBuilder &operator=(const MessageBuilder &) = delete;

    MessageBuilder();

    ~MessageBuilder();

    /// Deep memory cleanup
    void reset();

    /// Returns true if nothing added
    bool empty() const;

    /// Serializes into byte buffer and clears internal state
    outcome::result<SharedBuffer> serialize();

    /// Adds subscription notification
    void addSubscription(bool subscribe, const TopicId &topic);

    /// Adds "I have" notification
    void addIHave(const TopicId &topic, const MessageId &msg_id);

    /// Adds "I want" request
    void addIWant(const MessageId &msg_id);

    /// Adds graft request
    void addGraft(const TopicId &topic);

    /// Adds prune request
    void addPrune(const TopicId &topic);

    /// Adds message to be forwarded
    void addMessage(const TopicMessage &msg, const MessageId &msg_id);

    static outcome::result<ByteArray> signableMessage(const TopicMessage &msg);

   private:
    /// Creates protobuf structures if needed
    void create_protobuf_structures();

    /// Clears constructed message
    void clear();

    /// Protobuf message being constructed
    std::unique_ptr<pubsub::pb::RPC> pb_msg_;
    std::unique_ptr<pubsub::pb::ControlMessage> control_pb_msg_;
    bool empty_;
    bool control_not_empty_;

    /// Intermediate struct for building IHave messages
    std::map<TopicId, std::vector<MessageId>> ihaves_;

    /// Intermediate struct for building IWant request
    std::vector<MessageId> iwant_;

    /// Used to prevent duplicate forwarding
    std::unordered_set<MessageId> messages_added_;
  };
}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_MESSAGE_BUILDER_HPP
