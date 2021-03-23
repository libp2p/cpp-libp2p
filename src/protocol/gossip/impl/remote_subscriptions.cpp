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
                                           Scheduler &scheduler,
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
                                               TopicId topic) {
    if (peer->subscribed_to.erase(topic) == 0) {
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
    while (!peer->subscribed_to.empty()) {
      onPeerUnsubscribed(peer, *peer->subscribed_to.begin());
    }
  }

  bool RemoteSubscriptions::hasTopic(const TopicId &topic) const {
    return table_.count(topic) != 0;
  }

  bool RemoteSubscriptions::hasTopics(const TopicList &topics) const {
    for (const auto &topic : topics) {
      if (hasTopic(topic)) {
        return true;
      }
    }
    return false;
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
    res.value().onPrune(peer, scheduler_.now() + backoff_time * 1000);
  }

  void RemoteSubscriptions::onNewMessage(
      const boost::optional<PeerContextPtr> &from, const TopicMessage::Ptr &msg,
      const MessageId &msg_id) {
    auto now = scheduler_.now();
    bool is_published_locally = !from.has_value();
    for (const auto &topic : msg->topic_ids) {
      auto res = getItem(topic, is_published_locally);
      if (!res) {
        log_.error("error getting item for {}", topic);
        continue;
      }
      res.value().onNewMessage(from, msg, msg_id, now);
    }
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
