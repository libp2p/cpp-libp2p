/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/log/sublogger.hpp>

#include "topic_subscriptions.hpp"

namespace libp2p::protocol::gossip {

  /// Manages topic subscriptions from remote peers
  class RemoteSubscriptions {
   public:
    /// Ctor. Dependencies are passed by ref becaus this object is a part of
    /// GossipCore and lives only within its scope
    RemoteSubscriptions(const Config &config,
                        Connectivity &connectivity,
                        std::shared_ptr<Score> score,
                        std::shared_ptr<GossipPromises> gossip_promises,
                        std::shared_ptr<basic::Scheduler> scheduler,
                        log::SubLogger &log);

    /// This host subscribes or unsubscribes
    void onSelfSubscribed(bool subscribed, const TopicId &topic);

    /// Remote peer subscribes
    void onPeerSubscribed(const PeerContextPtr &peer, const TopicId &topic);

    /// Remote peer unsubscribes
    void onPeerUnsubscribed(const PeerContextPtr &peer,
                            const TopicId &topic,
                            bool disconnected);

    /// Peer disconnected - remove it from all topics it's subscribed to
    void onPeerDisconnected(const PeerContextPtr &peer);

    /// Returns if topic exists in the table
    bool isSubscribed(const TopicId &topic) const;

    /// Remote peer adds topic into its mesh
    void onGraft(const PeerContextPtr &peer, const TopicId &topic);

    /// Remote peer removes topic from its mesh
    void onPrune(const PeerContextPtr &peer,
                 const TopicId &topic,
                 std::optional<std::chrono::seconds> backoff_time);

    /// Forwards message to its topics. If 'from' is not set then the message is
    /// published locally
    void onNewMessage(const boost::optional<PeerContextPtr> &from,
                      const TopicMessage::Ptr &msg,
                      const MessageId &msg_id);

    /// Periodic job needed to update meshes and shift "I have" caches
    void onHeartbeat();

   private:
    /// Returns table item, creates a new one if needed
    boost::optional<TopicSubscriptions &> getItem(const TopicId &topic,
                                                  bool create_if_not_exist);

    const Config &config_;
    Connectivity &connectivity_;
    std::shared_ptr<ChoosePeers> choose_peers_;
    std::shared_ptr<ExplicitPeers> explicit_peers_;
    std::shared_ptr<Score> score_;
    std::shared_ptr<GossipPromises> gossip_promises_;
    std::shared_ptr<basic::Scheduler> scheduler_;

    std::unordered_map<TopicId, TopicSubscriptions> table_;

    log::SubLogger &log_;
  };

}  // namespace libp2p::protocol::gossip
