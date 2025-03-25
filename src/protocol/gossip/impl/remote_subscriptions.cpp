/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "remote_subscriptions.hpp"

#include <algorithm>
#include <libp2p/protocol/gossip/explicit_peers.hpp>

#include "choose_peers.hpp"
#include "connectivity.hpp"
#include "message_builder.hpp"

namespace libp2p::protocol::gossip {

  RemoteSubscriptions::RemoteSubscriptions(
      const Config &config,
      Connectivity &connectivity,
      std::shared_ptr<Score> score,
      std::shared_ptr<GossipPromises> gossip_promises,
      std::shared_ptr<basic::Scheduler> scheduler,
      log::SubLogger &log)
      : config_(config),
        connectivity_(connectivity),
        choose_peers_{std::make_shared<ChoosePeers>()},
        explicit_peers_{std::make_shared<ExplicitPeers>()},
        score_{std::move(score)},
        gossip_promises_{std::move(gossip_promises)},
        scheduler_{std::move(scheduler)},
        log_(log) {}

  void RemoteSubscriptions::onSelfSubscribed(bool subscribed,
                                             const TopicId &topic) {
    if (subscribed == isSubscribed(topic)) {
      return;
    }
    auto res = getItem(topic, subscribed);
    if (!res) {
      log_.error("error in self subscribe to {}", topic);
      return;
    }
    TopicSubscriptions &subs = res.value();
    if (subscribed) {
      subs.subscribe();
    } else {
      subs.unsubscribe();
    }
    if (not subs.isUsed()) {
      table_.erase(topic);
    }
    log_.debug("self {} {}",
               (subscribed ? "subscribed to" : "unsubscribed from"),
               topic);
  }

  void RemoteSubscriptions::onPeerSubscribed(const PeerContextPtr &peer,
                                             const TopicId &topic) {
    if (!peer->subscribed_to.insert(topic).second) {
      // request from wire, already subscribed, ignoring double subscription
      log_.debug("peer {} already subscribed to {}", peer->str, topic);
      return;
    }
    log_.debug("peer {} subscribing to {}", peer->str, topic);

    auto res = getItem(topic, false);
    if (!res) {
      // not error in this case, this is request from wire...
      log_.debug("entry doesnt exist for {}", topic);
      return;
    }
    TopicSubscriptions &subs = res.value();
    subs.onPeerSubscribed(peer);
  }

  void RemoteSubscriptions::onPeerUnsubscribed(const PeerContextPtr &peer,
                                               const TopicId &topic,
                                               bool disconnected) {
    if (!disconnected && peer->subscribed_to.erase(topic) == 0) {
      // was not subscribed actually, ignore
      log_.debug("peer {} was not subscribed to {}", peer->str, topic);
      return;
    }

    log_.debug("peer {} unsubscribing from {}", peer->str, topic);
    auto res = getItem(topic, false);
    if (!res) {
      // not error in this case, this is request from wire...
      log_.debug("entry doesnt exist for {}", topic);
      return;
    }

    TopicSubscriptions &subs = res.value();

    subs.onPeerUnsubscribed(peer);
    if (not subs.isUsed()) {
      table_.erase(topic);
    }
  }

  void RemoteSubscriptions::onPeerDisconnected(const PeerContextPtr &peer) {
    std::set<std::string> subscribed_to;
    subscribed_to.swap(peer->subscribed_to);

    for (const auto &topic : subscribed_to) {
      onPeerUnsubscribed(peer, topic, true);
    }
  }

  bool RemoteSubscriptions::isSubscribed(const TopicId &topic) const {
    auto it = table_.find(topic);
    return it != table_.end() and it->second.isSubscribed();
  }

  void RemoteSubscriptions::onGraft(const PeerContextPtr &peer,
                                    const TopicId &topic) {
    // implicit subscribe on graft
    peer->subscribed_to.insert(topic);
    auto res = getItem(topic, false);
    if (!res) {
      // we don't have this topic anymore
      peer->message_builder->addPrune(
          topic,
          peer->isGossipsubv1_1() ? std::make_optional(config_.prune_backoff)
                                  : std::nullopt);
      connectivity_.peerIsWritable(peer);
      return;
    }
    res.value().onGraft(peer);
  }

  void RemoteSubscriptions::onPrune(
      const PeerContextPtr &peer,
      const TopicId &topic,
      std::optional<std::chrono::seconds> backoff_time) {
    auto res = getItem(topic, false);
    if (!res) {
      return;
    }
    res.value().onPrune(peer, backoff_time);
  }

  void RemoteSubscriptions::onNewMessage(
      const boost::optional<PeerContextPtr> &from,
      const TopicMessage::Ptr &msg,
      const MessageId &msg_id) {
    auto now = scheduler_->now();
    bool is_published_locally = !from.has_value();
    auto res = getItem(msg->topic, is_published_locally);
    if (!res) {
      log_.error("error getting item for {}", msg->topic);
      return;
    }
    res.value().onNewMessage(from, msg, msg_id, now);
  }

  void RemoteSubscriptions::onHeartbeat() {
    auto now = scheduler_->now();
    for (auto it = table_.begin(); it != table_.end();) {
      it->second.onHeartbeat(now);
      if (not it->second.isUsed()) {
        // fanout interval expired - clean up
        log_.debug("deleted entry for topic {}", it->first);
        it = table_.erase(it);
      } else {
        ++it;
      }
    }
  }

  boost::optional<TopicSubscriptions &> RemoteSubscriptions::getItem(
      const TopicId &topic, bool create_if_not_exist) {
    auto it = table_.find(topic);
    if (it != table_.end()) {
      return it->second;
    }
    if (create_if_not_exist) {
      auto [it, _] = table_.emplace(topic,
                                    TopicSubscriptions(topic,
                                                       config_,
                                                       connectivity_,
                                                       scheduler_,
                                                       choose_peers_,
                                                       explicit_peers_,
                                                       score_,
                                                       gossip_promises_,
                                                       log_));
      TopicSubscriptions &item = it->second;
      log_.debug("created entry for topic {}", topic);
      return item;
    }
    return boost::none;
  }

}  // namespace libp2p::protocol::gossip
