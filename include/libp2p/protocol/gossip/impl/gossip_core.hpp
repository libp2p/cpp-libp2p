/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_CORE_HPP
#define LIBP2P_PROTOCOL_GOSSIP_CORE_HPP

#include <map>

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>
#include <libp2p/protocol/gossip/impl/message_cache.hpp>
#include <libp2p/protocol/gossip/impl/message_receiver.hpp>
#include <libp2p/protocol/gossip/impl/peer_set.hpp>

namespace libp2p::protocol::gossip {

  class LocalSubscriptions;
  class RemoteSubscriptions;
  class Connectivity;

  /// Central component in gossip protocol impl, manages pub-sub logic itself
  class GossipCore : public Gossip, public MessageReceiver,
                     public std::enable_shared_from_this<GossipCore>{
   public:
    GossipCore(const GossipCore &) = delete;
    GossipCore &operator=(const GossipCore &) = delete;
    GossipCore(GossipCore &&) = delete;
    GossipCore &operator=(GossipCore &&) = delete;

    GossipCore(Config config, std::shared_ptr<Scheduler> scheduler,
               std::shared_ptr<Host> host);

    ~GossipCore() override = default;

   private:
    // Gossip overrides
    void start() override;
    void stop() override;
    Subscription subscribe(TopicSet topics,
                           SubscriptionCallback callback) override;
    bool publish(const TopicSet &topic, ByteArray data) override;

    // MessageReceiver overrides
    void onSubscription(const PeerContextPtr &from, bool subscribe,
                        const TopicId &topic) override;
    void onIHave(const PeerContextPtr &from, const TopicId &topic,
                 const MessageId &msg_id) override;
    void onIWant(const PeerContextPtr &from, const MessageId &msg_id) override;
    void onGraft(const PeerContextPtr &from, const TopicId &topic) override;
    void onPrune(const PeerContextPtr &from, const TopicId &topic) override;
    void onTopicMessage(const PeerContextPtr &from,
                        TopicMessage::Ptr msg) override;
    void onMessageEnd(const PeerContextPtr &from) override;

    /// Periodic heartbeat
    void onHeartbeat();

    /// Lucal host subscribed or unsubscribed from topic
    void onLocalSubscriptionChanged(bool subscribe, const TopicId &topic);

    /// Remote peer connected or disconnected
    void onPeerConnection(bool connected, const PeerContextPtr &ctx);

    const Config config_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<Host> host_;
    peer::PeerId local_peer_id_;
    MessageCache msg_cache_;
    std::shared_ptr<LocalSubscriptions> local_subscriptions_;
    std::shared_ptr<RemoteSubscriptions> remote_subscriptions_;
    std::shared_ptr<Connectivity> connectivity_;
    std::map<TopicId, bool> broadcast_on_heartbeat_;
    uint64_t msg_seq_;
    bool started_ = false;

    /// Heartbeat timer handle
    Scheduler::Handle heartbeat_timer_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_CORE_HPP
