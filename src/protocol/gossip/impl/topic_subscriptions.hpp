/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_TOPIC_SUBSCRIPTIONS_HPP
#define LIBP2P_PROTOCOL_GOSSIP_TOPIC_SUBSCRIPTIONS_HPP

#include <deque>

#include <libp2p/log/sublogger.hpp>

#include "peer_set.hpp"

namespace libp2p::protocol::gossip {

  class Connectivity;

  /// Per-topic subscriptions
  class TopicSubscriptions {
   public:
    /// Ctor. Dependencies are passed by ref because this object is a part of
    /// RemoteSubscriptions and lives only within its scope
    TopicSubscriptions(TopicId topic, const Config &config,
                       Connectivity &connectivity, log::SubLogger &log);

    /// Returns true if no peers subscribed and not self-subscribed and
    /// no fanout period at the moment (empty item may be erased)
    bool empty() const;

    /// Forwards message to mesh members and announce to other subscribers
    void onNewMessage(const boost::optional<PeerContextPtr> &from,
                      const TopicMessage::Ptr &msg, const MessageId &msg_id,
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
    void onPrune(const PeerContextPtr &p, Time dont_bother_until);

   private:
    /// Adds a peer to mesh
    void addToMesh(const PeerContextPtr &p);

    /// Removes a peer from mesh
    void removeFromMesh(const PeerContextPtr &p);

    const TopicId topic_;
    const Config &config_;
    Connectivity &connectivity_;

    /// This host subscribed to this topic or not, this affects mesh behavior
    bool self_subscribed_;

    /// Fanout period allows for publishing from this host without subscribing
    Time fanout_period_ends_;

    /// Peers subscribed to this topic, but not mesh members
    PeerSet subscribed_peers_;

    /// Mesh members to whom messages are forwarded in push manner
    PeerSet mesh_peers_;

    /// "I have" notifications for new subscribers aka seen messages cache
    std::deque<std::pair<Time, MessageId>> seen_cache_;

    /// Prune backoff times per peer
    std::unordered_map<PeerContextPtr, Time> dont_bother_until_;

    log::SubLogger &log_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_TOPIC_SUBSCRIPTIONS_HPP
