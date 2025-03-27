/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <random>

#include <libp2p/log/sublogger.hpp>

#include "peer_set.hpp"

namespace libp2p::protocol::gossip {
  class ChoosePeers;
  class Connectivity;
  class ExplicitPeers;
  class Score;

  /// Per-topic subscriptions
  class TopicSubscriptions {
   public:
    /// Ctor. Dependencies are passed by ref because this object is a part of
    /// RemoteSubscriptions and lives only within its scope
    TopicSubscriptions(TopicId topic,
                       const Config &config,
                       Connectivity &connectivity,
                       std::shared_ptr<basic::Scheduler> scheduler,
                       std::shared_ptr<ChoosePeers> choose_peers,
                       std::shared_ptr<ExplicitPeers> explicit_peers,
                       std::shared_ptr<Score> score,
                       std::shared_ptr<OutboundPeers> outbound_peers,
                       log::SubLogger &log);

    /// Returns true if not self-subscribed and
    /// no fanout period at the moment (empty item may be erased)
    bool isUsed() const;

    bool isSubscribed() const;

    /// Forwards message to mesh members and announce to other subscribers
    void onNewMessage(const boost::optional<PeerContextPtr> &from,
                      const TopicMessage::Ptr &msg,
                      const MessageId &msg_id,
                      Time now);

    /// Periodic job needed to update meshes and shift "I have" caches
    void onHeartbeat(Time now);

    void subscribe();
    void unsubscribe();

    /// Remote peer subscribes to topic
    void onPeerSubscribed(const PeerContextPtr &p);

    /// Remote peer unsubscribes from topic
    void onPeerUnsubscribed(const PeerContextPtr &p);

    /// Remote peer includes this host into its mesh
    void onGraft(const PeerContextPtr &p);

    /// Remote peer kicks this host out of its mesh
    void onPrune(const PeerContextPtr &p,
                 std::optional<std::chrono::seconds> backoff);

   private:
    struct Fanout {
      Time until;
      PeerSet peers;
    };

    /// Adds a peer to mesh
    void addToMesh(const PeerContextPtr &p);

    /// Removes a peer from mesh
    void sendPrune(const PeerContextPtr &ctx, bool unsubscribe);

    void emitGossip();
    bool isBackoff(const PeerId &peer_id,
                   std::chrono::milliseconds slack) const;
    bool isBackoffWithSlack(const PeerId &peer_id) const;
    void updateBackoff(const PeerId &peer_id,
                       std::chrono::milliseconds duration);

    const TopicId topic_;
    const Config &config_;
    Connectivity &connectivity_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<ChoosePeers> choose_peers_;
    std::shared_ptr<ExplicitPeers> explicit_peers_;
    std::shared_ptr<Score> score_;
    std::shared_ptr<OutboundPeers> outbound_peers_;

    /// This host subscribed to this topic or not, this affects mesh behavior
    bool self_subscribed_;

    /// Fanout period allows for publishing from this host without subscribing
    std::optional<Fanout> fanout_;

    /// Peers subscribed to this topic, but not mesh members
    PeerSet subscribed_peers_;

    /// Mesh members to whom messages are forwarded in push manner
    PeerSet mesh_peers_;

    /// "I have" notifications for new subscribers
    std::deque<std::deque<MessageId>> history_gossip_;

    /// Prune backoff times per peer
    std::unordered_map<PeerId, Time> dont_bother_until_;

    log::SubLogger &log_;

    std::default_random_engine gossip_random_;
  };

}  // namespace libp2p::protocol::gossip
