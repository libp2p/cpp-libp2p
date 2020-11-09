/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_LOCAL_SUBSCRIPTIONS_HPP
#define LIBP2P_PROTOCOL_GOSSIP_LOCAL_SUBSCRIPTIONS_HPP

#include <map>

#include <libp2p/protocol/common/subscriptions.hpp>

#include "common.hpp"

namespace libp2p::protocol::gossip {

  /// Logic that manages topic subscriptions of this host
  class LocalSubscriptions : public SubscriptionsTo<Gossip::SubscriptionData> {
    using Super = SubscriptionsTo<Gossip::SubscriptionData>;

   public:
    using OnSubscriptionSetChange =
        std::function<void(bool subscribe, const TopicId &topic)>;

    explicit LocalSubscriptions(OnSubscriptionSetChange change_fn);

    /// Subscribes to topics
    Subscription subscribe(TopicSet topics,
                           Gossip::SubscriptionCallback callback);

    /// Returns all topics (and counters) this host is subscribed to
    const std::map<TopicId, size_t> &subscribedTo();

    /// Forwards data to subscriptions
    void forwardMessage(const TopicMessage::Ptr &msg);

    /// Forwards EOS to all subscribers
    void forwardEndOfSubscription();

   private:
    /// Returns if message is applicable to subscription ticket
    bool filter(uint64_t ticket, Gossip::SubscriptionData data) override;

    /// Unsubscribes from topics
    void unsubscribe(uint64_t ticket) override;

    /// Callback that called when this host subscribes to/unsubscribes from
    /// some topic
    OnSubscriptionSetChange change_fn_;

    /// Keeps track of topics this host is subscribed to
    std::map<TopicId, size_t> topics_;

    /// Used by filter()
    std::map<uint64_t, TopicSet> filters_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_LOCAL_SUBSCRIPTIONS_HPP
