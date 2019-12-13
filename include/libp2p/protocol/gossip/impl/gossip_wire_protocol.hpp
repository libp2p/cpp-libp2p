/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GOSSIP_WIRE_PROTOCOL_HPP
#define LIBP2P_GOSSIP_WIRE_PROTOCOL_HPP

#include <libp2p/protocol/gossip/common.hpp>

#include <map>

#include <gsl/span>

namespace pubsub::pb {
  class RPC;
  class ControlMessage;
}  // namespace pubsub::pb

namespace libp2p::protocol::gossip {

  /// Subscribe-unsubscribe request
  struct SubOpts {
    /// true means subscribe, false means unsubscribe
    bool subscribe = false;

    /// Pub-sub topic
    TopicId topic_id;
  };

  /// Announces about topic messages available on a host
  using IHaveTable = std::map<TopicId, Repeated<MessageId>>;

  /// General wire protocol message
  struct RPCMessage {
    /// Subscribe-unsubscribe requests
    Repeated<SubOpts> subscriptions;

    /// "I have" announces by topics
    IHaveTable i_have;

    /// Requests for messages
    Repeated<MessageId> i_want;

    /// Requests to join per-topic mesh
    Repeated<TopicId> graft;

    /// Requests to leave per-topic mesh
    Repeated<TopicId> prune;

    /// Messages to publish
    Repeated<TopicMessage::Ptr> publish;

    bool empty() const;

    void clear();

    bool deserialize(gsl::span<const uint8_t> bytes);

    bool serialize(std::vector<uint8_t> &buffer) const;
  };

  /// Interface for accepting sub messages being read from wire
  class MessageReceiver {
   public:
    virtual ~MessageReceiver() = default;

    /// Topic subscribe-unsubscribe request received
    virtual void onSubscription(const PeerId &from, bool subscribe,
                                const TopicId &topic) = 0;

    /// "I have message ids" notification received
    virtual void onIHave(const PeerId &from, const TopicId &topic,
                         MessageId &&msg_id) = 0;

    /// "I want message" request received
    virtual void onIWant(const PeerId &from, MessageId &&msg_id) = 0;

    /// Graft request received (gossip mesh control)
    virtual void onGraft(const PeerId &from, const TopicId &topic) = 0;

    /// Prune request received (gossip mesh control)
    virtual void onPrune(const PeerId &from, const TopicId &topic) = 0;

    /// Message received
    virtual void onMessage(const PeerId &from, TopicMessage::Ptr msg) = 0;
  };

  /// Parses RPC protobuf message received from wire
  bool parseRPCMessage(const PeerId &from, gsl::span<const uint8_t> bytes,
                       MessageReceiver &receiver);

  /// Constructs RPC message as new fields added and serializes it
  /// into bytes before sending into wire
  class MessageBuilder {
   public:
    MessageBuilder(MessageBuilder&&) = default;
    MessageBuilder& operator=(MessageBuilder&&) = default;
    MessageBuilder(const MessageBuilder&) = delete;
    MessageBuilder& operator=(const MessageBuilder&) = delete;

    MessageBuilder();

    ~MessageBuilder();

    /// Clears constructed message
    void clear();

    /// Serializes into byte buffer (appends to existing buffer) and clears
    bool serialize(std::vector<uint8_t> &buffer);

    /// Adds subsciption request to message
    void addSubscription(bool subscribe, const TopicId &topic);

    void addIHave(const TopicId &topic, const MessageId &msg_id);

    void addIWant(const MessageId &msg_id);

    void addGraft(const TopicId &topic);

    void addPrune(const TopicId &topic);

    void addMessage(const TopicMessage &msg);

   private:
    /// Protobuf message being constructed
    std::unique_ptr<pubsub::pb::RPC> pb_msg_;
    std::unique_ptr<pubsub::pb::ControlMessage> control_pb_msg_;
    bool control_not_empty_;

    /// Intermediate struct for building IHave messages
    IHaveTable ihaves_;

    /// Intermediate struct for building IWant request
    Repeated<MessageId> iwant_;
  };

  // TODO(artem): TopicDescriptor and rendezvous points

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_GOSSIP_WIRE_PROTOCOL_HPP
