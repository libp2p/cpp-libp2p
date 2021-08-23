/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gossip_core.hpp"

#include <cassert>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/peer/identity_manager.hpp>

#include "connectivity.hpp"
#include "local_subscriptions.hpp"
#include "message_builder.hpp"
#include "remote_subscriptions.hpp"

namespace libp2p::protocol::gossip {

  std::shared_ptr<Gossip> create(
      std::shared_ptr<basic::Scheduler> scheduler, std::shared_ptr<Host> host,
      std::shared_ptr<peer::IdentityManager> idmgr,
      std::shared_ptr<crypto::CryptoProvider> crypto_provider,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      Config config) {
    return std::make_shared<GossipCore>(std::move(config), std::move(scheduler),
                                        std::move(host), std::move(idmgr),
                                        std::move(crypto_provider),
                                        std::move(key_marshaller));
  }

  // clang-format off
  GossipCore::GossipCore(Config config,
                         std::shared_ptr<basic::Scheduler> scheduler,
                         std::shared_ptr<Host> host,
                         std::shared_ptr<peer::IdentityManager> idmgr,
                         std::shared_ptr<crypto::CryptoProvider> crypto_provider,
                         std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : config_(std::move(config)),
        create_message_id_([](const ByteArray &from, const ByteArray &seq,
                              const ByteArray &data){
          return createMessageId(from, seq, data);
        }),
        scheduler_(std::move(scheduler)),
        host_(std::move(host)),
        idmgr_(std::move(idmgr)),
        crypto_provider_(std::move(crypto_provider)),
        key_marshaller_(std::move(key_marshaller)),
        local_peer_id_(host_->getPeerInfo().id),
        msg_cache_(
            config_.message_cache_lifetime_msec,
            [sch = scheduler_] { return sch->now(); }
        ),
        local_subscriptions_(std::make_shared<LocalSubscriptions>(
            [this](bool subscribe, const TopicId &topic) {
              onLocalSubscriptionChanged(subscribe, topic);
            }
        )),
        msg_seq_(scheduler_->now().count()),
        log_("gossip", "Gossip", local_peer_id_.toBase58().substr(46)) {}
  // clang-format on

  void GossipCore::addBootstrapPeer(
      const peer::PeerId &id, boost::optional<multi::Multiaddress> address) {
    if (started_) {
      connectivity_->addBootstrapPeer(id, address);
    }
    bootstrap_peers_[id] = std::move(address);
  }

  outcome::result<void> GossipCore::addBootstrapPeer(
      const std::string &address) {
    OUTCOME_TRY(ma, libp2p::multi::Multiaddress::create(address));
    auto peer_id_str = ma.getPeerId();
    if (!peer_id_str) {
      return multi::Multiaddress::Error::INVALID_INPUT;
    }
    OUTCOME_TRY(peer_id, peer::PeerId::fromBase58(*peer_id_str));
    addBootstrapPeer(peer_id, {std::move(ma)});
    return outcome::success();
  }

  void GossipCore::start() {
    if (started_) {
      log_.warn("already started");
      return;
    }

    // clang-format off
    connectivity_ = std::make_shared<Connectivity>(
        config_,
        scheduler_,
        host_,
        shared_from_this(),
        [this](bool connected, const PeerContextPtr &ctx) {
          onPeerConnection(connected, ctx);
        }
    );
    // clang-format on

    for (const auto &p : bootstrap_peers_) {
      connectivity_->addBootstrapPeer(p.first, p.second);
    }

    remote_subscriptions_ = std::make_shared<RemoteSubscriptions>(
        config_, *connectivity_, *scheduler_, log_);

    started_ = true;

    for (const auto &[topic, _] : local_subscriptions_->subscribedTo()) {
      remote_subscriptions_->onSelfSubscribed(true, topic);
    }

    heartbeat_timer_ =
        scheduler_->scheduleWithHandle([this] { onHeartbeat(); });

    connectivity_->start();
  }

  void GossipCore::stop() {
    if (!started_) {
      return;
    }

    started_ = false;

    heartbeat_timer_.cancel();

    // it closes all senders and receivers
    connectivity_->stop();

    remote_subscriptions_.reset();
    connectivity_.reset();

    local_subscriptions_->forwardEndOfSubscription();
  }

  void GossipCore::setValidator(const TopicId &topic, Validator validator) {
    assert(validator);
    auto sub = subscribe({topic}, [](const SubscriptionData &) {});
    validators_[topic] = {std::move(validator), std::move(sub)};
  }

  void GossipCore::setMessageIdFn(MessageIdFn fn) {
    assert(fn);
    create_message_id_ = std::move(fn);
  }

  Subscription GossipCore::subscribe(TopicSet topics,
                                     SubscriptionCallback callback) {
    assert(callback);
    assert(!topics.empty());

    return local_subscriptions_->subscribe(std::move(topics),
                                           std::move(callback));
  }

  bool GossipCore::publish(TopicId topic, ByteArray data) {
    if (!started_) {
      return false;
    }

    auto msg = std::make_shared<TopicMessage>(
        local_peer_id_, ++msg_seq_, std::move(data), std::move(topic));

    if (config_.sign_messages) {
      auto res = signMessage(*msg);
      if (!res) {
        log_.warn("signMessage error: {}", res.error().message());
      }
    }

    MessageId msg_id = create_message_id_(msg->from, msg->seq_no, msg->data);

    [[maybe_unused]] bool inserted = msg_cache_.insert(msg, msg_id);
    assert(inserted);

    remote_subscriptions_->onNewMessage(boost::none, msg, msg_id);

    if (config_.echo_forward_mode) {
      local_subscriptions_->forwardMessage(msg);
    }

    return true;
  }

  outcome::result<void> GossipCore::signMessage(TopicMessage &msg) const {
    const auto &keypair = idmgr_->getKeyPair();
    OUTCOME_TRY(signable, MessageBuilder::signableMessage(msg));
    OUTCOME_TRY(signature,
                crypto_provider_->sign(signable, keypair.privateKey));
    msg.signature = std::move(signature);
    if (idmgr_->getId().toMultihash().getType() != multi::HashType::identity) {
      OUTCOME_TRY(key, key_marshaller_->marshal(keypair.publicKey));
      msg.key = std::move(key.key);
    }
    return outcome::success();
  }

  void GossipCore::onSubscription(const PeerContextPtr &peer, bool subscribe,
                                  const TopicId &topic) {
    assert(started_);

    log_.debug("peer {} {}subscribed, topic {}", peer->str,
               (subscribe ? "" : "un"), topic);
    if (subscribe) {
      remote_subscriptions_->onPeerSubscribed(peer, topic);
    } else {
      remote_subscriptions_->onPeerUnsubscribed(peer, topic, false);
    }
  }

  void GossipCore::onIHave(const PeerContextPtr &from, const TopicId &topic,
                           const MessageId &msg_id) {
    assert(started_);

    log_.debug("peer {} has msg for topic {}", from->str, topic);

    if (remote_subscriptions_->hasTopic(topic)
        && !msg_cache_.contains(msg_id)) {
      log_.debug("requesting msg id {}", common::hex_lower(msg_id));

      from->message_builder->addIWant(msg_id);
      connectivity_->peerIsWritable(from, false);
    }
  }

  void GossipCore::onIWant(const PeerContextPtr &from,
                           const MessageId &msg_id) {
    log_.debug("peer {} wants message {}", from->str,
               common::hex_lower(msg_id));

    auto msg_found = msg_cache_.getMessage(msg_id);
    if (msg_found) {
      from->message_builder->addMessage(*msg_found.value(), msg_id);
      connectivity_->peerIsWritable(from, true);
    } else {
      log_.debug("wanted message not in cache");
    }
  }

  void GossipCore::onGraft(const PeerContextPtr &from, const TopicId &topic) {
    assert(started_);

    log_.debug("graft from peer {} for topic {}", from->str, topic);

    remote_subscriptions_->onGraft(from, topic);
  }

  void GossipCore::onPrune(const PeerContextPtr &from, const TopicId &topic,
                           uint64_t backoff_time) {
    assert(started_);

    log_.debug("prune from peer {} for topic {}", from->str, topic);

    remote_subscriptions_->onPrune(from, topic, backoff_time);
  }

  void GossipCore::onTopicMessage(const PeerContextPtr &from,
                                  TopicMessage::Ptr msg) {
    assert(started_);

    // do we need this message?
    auto subscribed = remote_subscriptions_->hasTopic(msg->topic);
    if (!subscribed) {
      // ignore this message
      return;
    }

    MessageId msg_id = create_message_id_(msg->from, msg->seq_no, msg->data);
    log_.debug("message arrived, msg id={}", common::hex_lower(msg_id));

    if (msg_cache_.contains(msg_id)) {
      // already there, ignore
      log_.debug("ignoring message, already in cache");
      return;
    }

    // validate message. If no validator is set then we
    // suppose that the message is valid (we might not know topic details)
    bool valid = true;

    if (!validators_.empty()) {
      auto it = validators_.find(msg->topic);
      if (it != validators_.end()) {
        valid = it->second.validator(msg->from, msg->data);
      }
    }

    if (!valid) {
      log_.debug("message validation failed");
      return;
    }

    if (!msg_cache_.insert(msg, msg_id)) {
      log_.error("message cache error");
      return;
    }

    log_.debug("forwarding message");

    local_subscriptions_->forwardMessage(msg);
    remote_subscriptions_->onNewMessage(from, msg, msg_id);
  }

  void GossipCore::onMessageEnd(const PeerContextPtr &from) {
    assert(started_);

    log_.debug("finished dispatching message from peer {}", from->str);

    // Apply immediate send operation to affected peers
    connectivity_->flush();
  }

  void GossipCore::onHeartbeat() {
    assert(started_);

    // shift cache
    msg_cache_.shift();

    // heartbeat changes per topic
    remote_subscriptions_->onHeartbeat();

    // send changes to peers
    connectivity_->onHeartbeat(broadcast_on_heartbeat_);
    broadcast_on_heartbeat_.clear();

    auto res = heartbeat_timer_.reschedule(config_.heartbeat_interval_msec);
    if (!res) {
      log_.error("Heartbeat reschedule error: {}", res.error().message());
    }
  }

  void GossipCore::onPeerConnection(bool connected, const PeerContextPtr &ctx) {
    assert(started_);

    if (connected) {
      log_.debug("peer {} connected", ctx->str);
      // notify the new peer about all topics we subscribed to
      if (!local_subscriptions_->subscribedTo().empty()) {
        for (const auto &local_sub : local_subscriptions_->subscribedTo()) {
          ctx->message_builder->addSubscription(true, local_sub.first);
        }
        connectivity_->peerIsWritable(ctx, true);
        connectivity_->flush();
      }
    } else {
      log_.debug("peer {} disconnected", ctx->str);
      remote_subscriptions_->onPeerDisconnected(ctx);
    }
  }

  void GossipCore::onLocalSubscriptionChanged(bool subscribe,
                                              const TopicId &topic) {
    if (!started_) {
      return;
    }

    // send this notification on next heartbeat to all connected peers
    auto it = broadcast_on_heartbeat_.find(topic);
    if (it == broadcast_on_heartbeat_.end()) {
      broadcast_on_heartbeat_.emplace(topic, subscribe);
    } else if (it->second != subscribe) {
      // save traffic
      broadcast_on_heartbeat_.erase(it);
    }

    // update meshes per topic
    remote_subscriptions_->onSelfSubscribed(subscribe, topic);
  }

}  // namespace libp2p::protocol::gossip
