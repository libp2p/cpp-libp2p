/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gossip_core.hpp"

#include <cassert>

#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/protocol/gossip/score.hpp>
#include <qtils/hex.hpp>

#include "connectivity.hpp"
#include "local_subscriptions.hpp"
#include "message_builder.hpp"
#include "remote_subscriptions.hpp"

namespace libp2p::protocol::gossip {

  std::shared_ptr<Gossip> create(
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<Host> host,
      std::shared_ptr<peer::IdentityManager> idmgr,
      std::shared_ptr<crypto::CryptoProvider> crypto_provider,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      Config config) {
    return std::make_shared<GossipCore>(std::move(config),
                                        std::move(scheduler),
                                        std::move(host),
                                        std::move(idmgr),
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
        create_message_id_([](const Bytes &from, const Bytes &seq,
                              const Bytes &data){
          return createMessageId(from, seq, data);
        }),
        scheduler_(std::move(scheduler)),
        host_(std::move(host)),
        idmgr_(std::move(idmgr)),
        crypto_provider_(std::move(crypto_provider)),
        key_marshaller_(std::move(key_marshaller)),
        local_peer_id_(host_->getPeerInfo().id),
        msg_cache_(
            config_.history_length * config_.heartbeat_interval_msec,
            [sch = scheduler_] { return sch->now(); }
        ),
        score_{std::make_shared<Score>()},
        duplicate_cache_{config.duplicate_cache_time},
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
        config_, *connectivity_, score_, scheduler_, log_);

    started_ = true;

    for (const auto &[topic, _] : local_subscriptions_->subscribedTo()) {
      remote_subscriptions_->onSelfSubscribed(true, topic);
    }

    setTimerHeartbeat();

    connectivity_->start();
  }

  void GossipCore::stop() {
    if (!started_) {
      return;
    }

    started_ = false;

    heartbeat_timer_.reset();

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

  bool GossipCore::publish(TopicId topic, Bytes data) {
    if (!started_) {
      return false;
    }

    auto msg = std::make_shared<TopicMessage>(
        local_peer_id_, ++msg_seq_, std::move(data), std::move(topic));

    if (config_.sign_messages) {
      auto res = signMessage(*msg);
      if (!res) {
        log_.warn("signMessage error: {}", res.error());
      }
    }

    MessageId msg_id = create_message_id_(msg->from, msg->seq_no, msg->data);

    if (not duplicate_cache_.insert(msg_id)) {
      return false;
    }

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

  void GossipCore::onSubscription(const PeerContextPtr &peer,
                                  bool subscribe,
                                  const TopicId &topic) {
    assert(started_);

    log_.debug("peer {} {}subscribed, topic {}",
               peer->str,
               (subscribe ? "" : "un"),
               topic);
    if (subscribe) {
      remote_subscriptions_->onPeerSubscribed(peer, topic);
    } else {
      remote_subscriptions_->onPeerUnsubscribed(peer, topic, false);
    }
  }

  void GossipCore::onIHave(const PeerContextPtr &from,
                           const TopicId &topic,
                           const MessageId &msg_id) {
    assert(started_);

    if (not from->isGossipsub()) {
      return;
    }
    if (score_->below(from->peer_id, config_.score.gossip_threshold)) {
      return;
    }

    log_.debug("peer {} has msg for topic {}", from->str, topic);

    if (remote_subscriptions_->isSubscribed(topic)
        and not duplicate_cache_.contains(msg_id)) {
      log_.debug("requesting msg id {:x}", msg_id);

      from->message_builder->addIWant(msg_id);
      connectivity_->peerIsWritable(from);
    }
  }

  void GossipCore::onIWant(const PeerContextPtr &from,
                           const MessageId &msg_id) {
    if (not from->isGossipsub()) {
      return;
    }
    if (score_->below(from->peer_id, config_.score.gossip_threshold)) {
      return;
    }

    log_.debug("peer {} wants message {:x}", from->str, msg_id);

    auto msg_found = msg_cache_.getMessage(msg_id);
    if (msg_found) {
      from->message_builder->addMessage(*msg_found.value(), msg_id);
      connectivity_->peerIsWritable(from);
    } else {
      log_.debug("wanted message not in cache");
    }
  }

  void GossipCore::onGraft(const PeerContextPtr &from, const TopicId &topic) {
    assert(started_);

    if (not from->isGossipsub()) {
      return;
    }

    log_.debug("graft from peer {} for topic {}", from->str, topic);

    remote_subscriptions_->onGraft(from, topic);
  }

  void GossipCore::onPrune(const PeerContextPtr &from,
                           const TopicId &topic,
                           std::optional<std::chrono::seconds> backoff_time) {
    assert(started_);

    if (not from->isGossipsub()) {
      return;
    }

    log_.debug("prune from peer {} for topic {}", from->str, topic);

    remote_subscriptions_->onPrune(from, topic, backoff_time);
  }

  void GossipCore::onTopicMessage(const PeerContextPtr &from,
                                  TopicMessage::Ptr msg) {
    assert(started_);

    // do we need this message?
    auto subscribed = remote_subscriptions_->isSubscribed(msg->topic);
    if (!subscribed) {
      // ignore this message
      return;
    }

    MessageId msg_id = create_message_id_(msg->from, msg->seq_no, msg->data);
    if (not duplicate_cache_.insert(msg_id)) {
      return;
    }

    log_.debug("message arrived, msg id={:x}", msg_id);

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
  }

  void GossipCore::onHeartbeat() {
    assert(started_);

    // shift cache
    msg_cache_.shift();

    // heartbeat changes per topic
    remote_subscriptions_->onHeartbeat();

    // send changes to peers
    connectivity_->onHeartbeat();

    setTimerHeartbeat();
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
        connectivity_->peerIsWritable(ctx);
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

    connectivity_->subscribe(topic, subscribe);

    // update meshes per topic
    remote_subscriptions_->onSelfSubscribed(subscribe, topic);
  }

  void GossipCore::setTimerHeartbeat() {
    heartbeat_timer_ = scheduler_->scheduleWithHandle(
        [weak_self{weak_from_this()}] {
          auto self = weak_self.lock();
          if (not self) {
            return;
          }
          self->onHeartbeat();
        },
        config_.heartbeat_interval_msec);
  }
}  // namespace libp2p::protocol::gossip
