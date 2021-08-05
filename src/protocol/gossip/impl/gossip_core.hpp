/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_CORE_HPP
#define LIBP2P_PROTOCOL_GOSSIP_CORE_HPP

#include <libp2p/protocol/gossip/gossip.hpp>

#include <map>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/log/sublogger.hpp>

#include "message_cache.hpp"
#include "message_receiver.hpp"
#include "peer_set.hpp"

namespace libp2p::protocol::gossip {

  class LocalSubscriptions;
  class RemoteSubscriptions;
  class Connectivity;

  /// Central component in gossip protocol impl, manages pub-sub logic itself
  class GossipCore : public Gossip,
                     public MessageReceiver,
                     public std::enable_shared_from_this<GossipCore> {
   public:
    GossipCore(const GossipCore &) = delete;
    GossipCore &operator=(const GossipCore &) = delete;
    GossipCore(GossipCore &&) = delete;
    GossipCore &operator=(GossipCore &&) = delete;

    GossipCore(
        Config config, std::shared_ptr<basic::Scheduler> scheduler,
        std::shared_ptr<Host> host,
        std::shared_ptr<peer::IdentityManager> idmgr,
        std::shared_ptr<crypto::CryptoProvider> crypto_provider,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller);

    ~GossipCore() override = default;

   private:
    // Gossip overrides
    void addBootstrapPeer(
        const peer::PeerId &id,
        boost::optional<multi::Multiaddress> address) override;
    outcome::result<void> addBootstrapPeer(const std::string &address) override;
    void start() override;
    void stop() override;
    void setValidator(const TopicId &topic, Validator validator) override;
    void setMessageIdFn(MessageIdFn fn) override;
    Subscription subscribe(TopicSet topics,
                           SubscriptionCallback callback) override;
    bool publish(TopicId topic, ByteArray data) override;

    outcome::result<void> signMessage(TopicMessage &msg) const;

    // MessageReceiver overrides
    void onSubscription(const PeerContextPtr &from, bool subscribe,
                        const TopicId &topic) override;
    void onIHave(const PeerContextPtr &from, const TopicId &topic,
                 const MessageId &msg_id) override;
    void onIWant(const PeerContextPtr &from, const MessageId &msg_id) override;
    void onGraft(const PeerContextPtr &from, const TopicId &topic) override;
    void onPrune(const PeerContextPtr &from, const TopicId &topic,
                 uint64_t backoff_time) override;
    void onTopicMessage(const PeerContextPtr &from,
                        TopicMessage::Ptr msg) override;
    void onMessageEnd(const PeerContextPtr &from) override;

    /// Periodic heartbeat timer fn
    void onHeartbeat();

    /// Local host subscribed or unsubscribed from topic
    void onLocalSubscriptionChanged(bool subscribe, const TopicId &topic);

    /// Remote peer connected or disconnected
    void onPeerConnection(bool connected, const PeerContextPtr &ctx);

    /// Configuration parameters
    const Config config_;

    /// Message ID function
    MessageIdFn create_message_id_;

    /// Bootstrap peers to dial to
    std::unordered_map<peer::PeerId, boost::optional<multi::Multiaddress>>
        bootstrap_peers_;

    /// Scheduler for timers and async calls
    std::shared_ptr<basic::Scheduler> scheduler_;

    /// Host (interface to libp2p network)
    std::shared_ptr<Host> host_;

    std::shared_ptr<peer::IdentityManager> idmgr_;

    std::shared_ptr<crypto::CryptoProvider> crypto_provider_;

    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;

    /// This peer's id
    peer::PeerId local_peer_id_;

    /// Message cache w/expiration
    MessageCache msg_cache_;

    /// Local subscriptions manager (this host subscribed to topics)
    std::shared_ptr<LocalSubscriptions> local_subscriptions_;

    /// Remote subscriptions manager (other peers subscribed to topics)
    std::shared_ptr<RemoteSubscriptions> remote_subscriptions_;

    struct ValidatorAndLocalSub {
      Validator validator;
      Subscription sub;
    };

    /// Remote messages validators by topic
    std::unordered_map<TopicId, ValidatorAndLocalSub> validators_;

    /// Network part of gossip component
    std::shared_ptr<Connectivity> connectivity_;

    /// Local {un}subscribe changes to be broadcasted to peers
    std::map<TopicId, bool> broadcast_on_heartbeat_;

    /// Incremented msg sequence number
    uint64_t msg_seq_;

    /// True if started and active
    bool started_ = false;

    /// Heartbeat timer handle
    basic::Scheduler::Handle heartbeat_timer_;

    /// Logger
    log::SubLogger log_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_CORE_HPP
