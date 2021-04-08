/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_MESSAGE_RECEIVER_HPP
#define LIBP2P_PROTOCOL_GOSSIP_MESSAGE_RECEIVER_HPP

#include "common.hpp"

namespace libp2p::protocol::gossip {

  /// Interface for accepting sub-messages read from wire
  class MessageReceiver {
   public:
    virtual ~MessageReceiver() = default;

    /// Topic subscribe-unsubscribe request received
    virtual void onSubscription(const PeerContextPtr &from, bool subscribe,
                                const TopicId &topic) = 0;

    /// "I have message ids" notification received
    virtual void onIHave(const PeerContextPtr &from, const TopicId &topic,
                         const MessageId &msg_id) = 0;

    /// "I want message" request received
    virtual void onIWant(const PeerContextPtr &from,
                         const MessageId &msg_id) = 0;

    /// Graft request received (gossip mesh control)
    virtual void onGraft(const PeerContextPtr &from, const TopicId &topic) = 0;

    /// Prune request received (gossip mesh control).
    /// the peer must not be bothered with GRAFT requests for at least
    /// backoff_time seconds
    virtual void onPrune(const PeerContextPtr &from, const TopicId &topic,
                         uint64_t backoff_time) = 0;

    /// Message received
    virtual void onTopicMessage(const PeerContextPtr &from,
                                TopicMessage::Ptr msg) = 0;

    /// Current wire protocol message dispatch ended
    virtual void onMessageEnd(const PeerContextPtr &from) = 0;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_MESSAGE_RECEIVER_HPP
