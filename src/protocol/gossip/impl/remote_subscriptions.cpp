/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "remote_subscriptions.hpp"

#include <algorithm>

#include "connectivity.hpp"
#include "message_builder.hpp"

namespace libp2p::protocol::gossip {

  RemoteSubscriptions::RemoteSubscriptions(const Config &config,
                                           Connectivity &connectivity,
                                           basic::Scheduler &scheduler,
                                           log::SubLogger &log)
      : config_(config),
        connectivity_(connectivity),
        scheduler_(scheduler),
        log_(log) {}

  void RemoteSubscriptions::onSelfSubscribed(bool subscribed,
                                             const TopicId &topic) {
    auto res = getItem(topic, subscribed);
    if (!res) {
      log_.error("error in self subscribe to {}", topic);
      return;
    }
    TopicSubscriptions &subs = res.value();
    subs.onSelfSubscribed(subscribed);
    if (subs.empty()) {
      table_.erase(topic);
    }
    log_.debug("self {} {}",
               (subscribed ? "subscribed to" : "unsubscribed from"), topic);
  }

  void RemoteSubscriptions::onPeerSubscribed(const PeerContextPtr &peer,
                                             const TopicId &topic) {
    if (!peer->subscribed_to.insert(topic).second) {
      // request from wire, already subscribed, ignoring double subscription
      log_.debug("peer {} already subscribed to {}", peer->str, topic);
      return;
    }
    log_.debug("peer {} subscribing to {}", peer->str, topic);

    auto res = getItem(topic, true);
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
    if (subs.empty()) {
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

  bool RemoteSubscriptions::hasTopic(const TopicId &topic) const {
    return table_.count(topic) != 0;
  }

  void RemoteSubscriptions::onGraft(const PeerContextPtr &peer,
                                    const TopicId &topic) {
    auto res = getItem(topic, false);
    if (!res) {
      // we don't have this topic anymore
      peer->message_builder->addPrune(topic);
      connectivity_.peerIsWritable(peer, true);
      return;
    }
    res.value().onGraft(peer);
  }

  void RemoteSubscriptions::onPrune(const PeerContextPtr &peer,
                                    const TopicId &topic,
                                    uint64_t backoff_time) {
    auto res = getItem(topic, false);
    if (!res) {
      return;
    }
    res.value().onPrune(peer,
                        scheduler_.now() + std::chrono::seconds(backoff_time));
  }

  void RemoteSubscriptions::onNewMessage(
      const boost::optional<PeerContextPtr> &from, const TopicMessage::Ptr &msg,
      const MessageId &msg_id) {
    auto now = scheduler_.now();
    bool is_published_locally = !from.has_value();
    auto res = getItem(msg->topic, is_published_locally);
    if (!res) {
      log_.error("error getting item for {}", msg->topic);
      return;
    }
    res.value().onNewMessage(from, msg, msg_id, now);
  }

  void RemoteSubscriptions::onHeartbeat() {
    auto now = scheduler_.now();
    for (auto it = table_.begin(); it != table_.end();) {
      it->second.onHeartbeat(now);
      if (it->second.empty()) {
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
      auto [it, _] = table_.emplace(
          topic, TopicSubscriptions(topic, config_, connectivity_, log_));
      TopicSubscriptions &item = it->second;
      connectivity_.getConnectedPeers().selectIf(
          [&item](const PeerContextPtr &ctx) { item.onPeerSubscribed(ctx); },
          [&topic](const PeerContextPtr &ctx) {
            return ctx->subscribed_to.count(topic) != 0;
          });
      log_.debug("created entry for topic {}", topic);
      return item;
    }
    return boost::none;
  }

}  // namespace libp2p::protocol::gossip
