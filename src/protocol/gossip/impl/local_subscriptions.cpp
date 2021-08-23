/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "local_subscriptions.hpp"

#include <cassert>

namespace libp2p::protocol::gossip {
  LocalSubscriptions::LocalSubscriptions(OnSubscriptionSetChange change_fn)
      : change_fn_(std::move(change_fn)) {}

  Subscription LocalSubscriptions::subscribe(
      TopicSet topics, Gossip::SubscriptionCallback callback) {
    Subscription ret = Super::subscribe(std::move(callback));

    for (const auto &t : topics) {
      if (++topics_[t] == 1) {
        change_fn_(true, t);
      }
    }
    filters_[lastTicket()] = std::move(topics);

    return ret;
  }

  const std::map<TopicId, size_t> &LocalSubscriptions::subscribedTo() {
    return topics_;
  }

  void LocalSubscriptions::forwardMessage(const TopicMessage::Ptr &msg) {
    assert(msg);
    if (topics_.count(msg->topic) != 0) {
      Gossip::Message tmp_msg{msg->from, msg->topic, msg->data};
      publish(tmp_msg);
    }
  }

  void LocalSubscriptions::forwardEndOfSubscription() {
    publish(boost::none);
  }

  bool LocalSubscriptions::filter(uint64_t ticket,
                                  Gossip::SubscriptionData data) {
    if (!data) {
      // this is the end message, broadcast to all subscriptions
      return true;
    }
    auto it = filters_.find(ticket);

    assert(it != filters_.end());

    return it->second.count(data.value().topic) != 0;
  }

  void LocalSubscriptions::unsubscribe(uint64_t ticket) {
    Super::unsubscribe(ticket);

    auto it = filters_.find(ticket);
    if (it != filters_.end()) {
      TopicSet &s = it->second;
      for (auto topics_it = topics_.begin(); topics_it != topics_.end();) {
        if (s.count(topics_it->first) != 0) {
          if (--topics_it->second == 0) {
            change_fn_(false, topics_it->first);
            topics_it = topics_.erase(topics_it);
            continue;
          }
        }
        ++topics_it;
      }
      filters_.erase(it);
    }
  }

}  // namespace libp2p::protocol::gossip
