/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_REMOTE_SUBSCRIPTIONS_HPP
#define LIBP2P_PROTOCOL_GOSSIP_REMOTE_SUBSCRIPTIONS_HPP

#include <deque>

#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/gossip/impl/peers.hpp>

namespace libp2p::protocol::gossip {

  class Connectivity;

  /// Manages topic subscriptions from remote peers
  class RemoteSubscriptions {
    /// Per-topic subscriptions
    class TopicSubscriptions {
     public:
      /// Ctor. Dependencies are passed by ref becaus this object is a part of
      /// RemoteSubscriptions and lives only within its scope
      TopicSubscriptions(TopicId topic, const Config &config,
                         Connectivity &connectivity);

      /// Returns true if no peers subscribed and not self-subscribed and
      /// no fanout period at the moment (empty item may be erased)
      bool empty();

      /// Forwards message to mesh members and announce to other subscribers
      void onNewMessage(const TopicMessage::Ptr &msg,
                        const MessageId &msg_id, bool is_published_locally,
                        Time now);

      /// Periodic job needed to update meshes and shift "I have" caches
      void onHeartbeat(Time now);

      /// Local host subscribes or unsubscribes, this affects mesh
      void onSelfSubscribed(bool self_subscribed);

      /// Remote peer subscribes to topic
      void onPeerSubscribed(const PeerContextPtr &p);

      /// Remote peer unsubscribes from topic
      void onPeerUnsubscribed(const PeerContextPtr &p);

      /// Remote peer includes this host into its mesh
      void onGraft(const PeerContextPtr &p);

      /// Remote peer kicks this host out of its mesh
      void onPrune(const PeerContextPtr &p);

     private:
      /// Adds a peer to mesh
      void addToMesh(const PeerContextPtr &p);

      /// Removes a peer from mesh
      void removeFromMesh(const PeerContextPtr &p);

      const TopicId topic_;
      const Config &config_;
      Connectivity &connectivity_;

      /// This host subscribed to this topic or not, affects mesh behavior
      bool self_subscribed_;

      /// Fanout period allows for publishing from this host without subscribing
      Time fanout_period_ends_;

      /// Peers subscribed to this topic, but not mesh members
      PeerSet subscribed_peers_;

      /// Mesh members to whom messages are forwarded in push manner
      PeerSet mesh_peers_;

      /// "I have" notifications for new subscribers aka seen messages cache
      std::deque<std::pair<Time, MessageId>> seen_cache_;
    };

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
    bool hasTopic(const TopicId &topic);

    /// Returns if at least one of topics exists in the table
    bool hasTopics(const TopicList &topics);

    /// Remote peer adds topic into its mesh
    void onGraft(const PeerContextPtr &peer, const TopicId &topic);

    /// Remote peer removes topic from its mesh
    void onPrune(const PeerContextPtr &peer, const TopicId &topic);

    /// Forwards message to its topics
    void forwardMessage(const TopicMessage::Ptr &msg, const MessageId &msg_id,
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
