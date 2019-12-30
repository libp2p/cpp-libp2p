/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/remote_subscriptions.hpp>

#include <algorithm>

#include <libp2p/protocol/gossip/impl/connectivity.hpp>
#include <libp2p/protocol/gossip/impl/message_builder.hpp>

namespace libp2p::protocol::gossip {

  RemoteSubscriptions::RemoteSubscriptions(const Config &config,
                                           Connectivity &connectivity,
                                           Scheduler &scheduler)
      : config_(config), connectivity_(connectivity), scheduler_(scheduler) {}

  void RemoteSubscriptions::onSelfSubscribed(bool subscribed,
                                             const TopicId &topic) {
    auto res = getItem(topic, subscribed);
    if (!res) {
      // TODO(artem): log error
      return;
    }
    TopicSubscriptions &subs = res.value();
    subs.onSelfSubscribed(subscribed);
    if (subs.empty()) {
      table_.erase(topic);
    }
  }

  void RemoteSubscriptions::onPeerSubscribed(const PeerContextPtr &peer,
                                             bool subscribed,
                                             const TopicId &topic) {
    auto res = getItem(topic, subscribed);
    if (!res) {
      // not error in this case, this is request from wire...
      return;
    }
    TopicSubscriptions &subs = res.value();

    if (subscribed) {
      subs.onPeerSubscribed(peer);
    } else {
      subs.onPeerUnsubscribed(peer);
      if (subs.empty()) {
        table_.erase(topic);
      }
    }
  }

  void RemoteSubscriptions::onPeerDisconnected(const PeerContextPtr &peer) {
    for (const auto &topic : peer->subscribed_to) {
      onPeerSubscribed(peer, false, topic);
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
      peer->message_to_send->addPrune(topic);
      connectivity_.peerIsWritable(peer, true);
      return;
    }
    res.value().onGraft(peer);
  }

  void RemoteSubscriptions::onPrune(const PeerContextPtr &peer,
                                    const TopicId &topic) {
    auto res = getItem(topic, false);
    if (!res) {
      return;
    }
    res.value().onPrune(peer);
  }

  void RemoteSubscriptions::onNewMessage(const TopicMessage::Ptr &msg,
                                         const MessageId &msg_id,
                                         bool is_published_locally) {
    auto now = scheduler_.now();
    for (const auto &topic : msg->topic_ids) {
      auto res = getItem(topic, is_published_locally);
      if (!res) {
        // TODO(artem): log it. if (is_published_locally) then this is error
        continue;
      }
      res.value().onNewMessage(msg, msg_id, is_published_locally, now);
    }
  }

  void RemoteSubscriptions::onHeartbeat() {
    auto now = scheduler_.now();
    for (auto it = table_.begin(); it != table_.end();) {
      it->second.onHeartbeat(now);
      if (it->second.empty()) {
        // fanout interval expired - clean up
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
          topic, TopicSubscriptions(topic, config_, connectivity_));
      return it->second;
    }
    return boost::none;
  }

}  // namespace libp2p::protocol::gossip
