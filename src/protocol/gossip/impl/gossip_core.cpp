/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/gossip_core.hpp>

#include <cassert>

#include <libp2p/protocol/gossip/impl/connectivity.hpp>
#include <libp2p/protocol/gossip/impl/local_subscriptions.hpp>
#include <libp2p/protocol/gossip/impl/message_builder.hpp>
#include <libp2p/protocol/gossip/impl/remote_subscriptions.hpp>

namespace libp2p::protocol::gossip {

  // clang-format off
  GossipCore::GossipCore(Config config, std::shared_ptr<Scheduler> scheduler,
                         std::shared_ptr<Host> host)
      : config_(std::move(config)),
        scheduler_(std::move(scheduler)),
        host_(std::move(host)),
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
        msg_seq_(scheduler_->now()),
        log_("gossip")
  {
    log_.setInstanceName("Gossip", local_peer_id_.toBase58().substr(46));
  }
  // clang-format on

  void GossipCore::addBootstrapPeer(
      peer::PeerId id, boost::optional<multi::Multiaddress> address) {
    bootstrap_peers_[id] = address;
    if (started_) {
      connectivity_->addBootstrapPeer(std::move(id), std::move(address));
    }
  }

  void GossipCore::start() {
    if (started_) {
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

    // clang-format off
    heartbeat_timer_ = scheduler_->schedule(config_.heartbeat_interval_msec,
        [self_wptr=weak_from_this()] {
          auto self = self_wptr.lock();
          if (self) {
            self->onHeartbeat();
          }
        }
    );
    // clang-format on

    connectivity_->start();

    onHeartbeat();
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

  Subscription GossipCore::subscribe(TopicSet topics,
                                     SubscriptionCallback callback) {
    assert(callback);
    assert(!topics.empty());

    return local_subscriptions_->subscribe(std::move(topics),
                                           std::move(callback));
  }

  bool GossipCore::publish(const TopicSet &topics, ByteArray data) {
    if (!started_ || topics.empty()) {
      return false;
    }

    auto msg = std::make_shared<TopicMessage>(local_peer_id_, ++msg_seq_,
                                              std::move(data));

    msg->topic_ids.assign(topics.begin(), topics.end());

    // TODO(artem): validate msg

    MessageId msg_id = createMessageId(*msg);

    bool inserted = msg_cache_.insert(msg, msg_id);
    assert(inserted);

    remote_subscriptions_->onNewMessage(boost::none, msg, msg_id);

    if (config_.echo_forward_mode) {
      local_subscriptions_->forwardMessage(msg);
    }

    return true;
  }

  void GossipCore::onSubscription(const PeerContextPtr &peer, bool subscribe,
                                  const TopicId &topic) {
    assert(started_);

    log_.debug("peer {} {}subscribed, topic {}", peer->str,
               (subscribe ? "" : "un"), topic);
    remote_subscriptions_->onPeerSubscribed(peer, subscribe, topic);
  }

  void GossipCore::onIHave(const PeerContextPtr &from, const TopicId &topic,
                           const MessageId &msg_id) {
    assert(started_);

    log_.debug("peer {} has msg for topic {}", from->str, topic);

    if (remote_subscriptions_->hasTopic(topic)
        && !msg_cache_.contains(msg_id)) {
      from->message_to_send->addIWant(msg_id);
      connectivity_->peerIsWritable(from, false);
    }
  }

  void GossipCore::onIWant(const PeerContextPtr &from,
                           const MessageId &msg_id) {
    log_.debug("peer {} wants message", from->str);

    auto msg_found = msg_cache_.getMessage(msg_id);
    if (msg_found) {
      from->message_to_send->addMessage(*msg_found.value(), msg_id);
      connectivity_->peerIsWritable(from, true);
    } else {
      log_.warn("wanted message not in cache");
    }
  }

  void GossipCore::onGraft(const PeerContextPtr &from, const TopicId &topic) {
    assert(started_);

    log_.debug("graft from peer {} for topic {}", from->str, topic);

    remote_subscriptions_->onGraft(from, topic);
  }

  void GossipCore::onPrune(const PeerContextPtr &from, const TopicId &topic) {
    assert(started_);

    log_.debug("prune from peer {} for topic {}", from->str, topic);

    remote_subscriptions_->onPrune(from, topic);
  }

  void GossipCore::onTopicMessage(const PeerContextPtr &from,
                                  TopicMessage::Ptr msg) {
    assert(started_);

    // do we need this message?
    auto subscribed = remote_subscriptions_->hasTopics(msg->topic_ids);
    if (!subscribed) {
      // ignore this message
      return;
    }

    // TODO(artem): validate

    MessageId msg_id = createMessageId(*msg);
    if (!msg_cache_.insert(msg, msg_id)) {
      // already there, ignore
      log_.debug("ignoring message from peer {}, already in cache", from->str);
      return;
    }

    log_.debug("forwarding message from peer {}", from->str);

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

    heartbeat_timer_.reschedule(config_.heartbeat_interval_msec);
  }

  void GossipCore::onPeerConnection(bool connected, const PeerContextPtr &ctx) {
    assert(started_);


    if (connected) {
      log_.debug("peer {} connected", ctx->str);
      // notify the new peer about all topics we subscribed to
      if (!local_subscriptions_->subscribedTo().empty()) {
        for (auto &local_sub : local_subscriptions_->subscribedTo()) {
          ctx->message_to_send->addSubscription(true, local_sub.first);
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
