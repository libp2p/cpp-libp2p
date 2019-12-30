/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_REMOTE_SUBSCRIPTIONS_HPP
#define LIBP2P_PROTOCOL_GOSSIP_REMOTE_SUBSCRIPTIONS_HPP

#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/gossip/impl/topic_subscriptions.hpp>

namespace libp2p::protocol::gossip {

  /// Manages topic subscriptions from remote peers
  class RemoteSubscriptions {
   public:
    /// Ctor. Dependencies are passed by ref becaus this object is a part of
    /// GossipCore and lives only within its scope
    RemoteSubscriptions(const Config &config, Connectivity &connectivity,
                        Scheduler &scheduler);

    /// This host subscribes or unsubscribes
    void onSelfSubscribed(bool subscribed, const TopicId &topic);

    /// Remote peer subscribes or unsubscribes
    void onPeerSubscribed(const PeerContextPtr &peer, bool subscribed,
                          const TopicId &topic);

    /// Peer disconnected - remove it from all topics it's subscribed to
    void onPeerDisconnected(const PeerContextPtr &peer);

    /// Returns if topic exists in the table
    bool hasTopic(const TopicId &topic) const;

    /// Returns if at least one of topics exists in the table
    bool hasTopics(const TopicList &topics) const;

    /// Remote peer adds topic into its mesh
    void onGraft(const PeerContextPtr &peer, const TopicId &topic);

    /// Remote peer removes topic from its mesh
    void onPrune(const PeerContextPtr &peer, const TopicId &topic);

    /// Forwards message to its topics
    void onNewMessage(const TopicMessage::Ptr &msg, const MessageId &msg_id,
                      bool is_published_locally);

    /// Periodic job needed to update meshes and shift "I have" caches
    void onHeartbeat();

   private:
    /// Returns table item, creates a new one if needed
    boost::optional<TopicSubscriptions &> getItem(const TopicId &topic,
                                                  bool create_if_not_exist);

    const Config &config_;
    Connectivity &connectivity_;
    Scheduler &scheduler_;

    // TODO(artem): bound table size (which may grow!)
    // by removing items not subscribed to locally. LRU(???)
    std::unordered_map<TopicId, TopicSubscriptions> table_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_REMOTE_SUBSCRIPTIONS_HPP
