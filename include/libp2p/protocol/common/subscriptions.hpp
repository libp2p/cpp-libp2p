/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_SUBSCRIPTIONS_HPP
#define LIBP2P_PROTOCOL_SUBSCRIPTIONS_HPP

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <libp2p/protocol/common/subscription.hpp>

namespace libp2p::protocol {

  /// Set of subscriptions, re-entrancy is allowed, exceptions aren't
  template <typename... Args>
  class SubscriptionsTo : public Subscription::Source {
   public:
    /// Subscription callback
    using Callback = std::function<void(Args...)>;

    ~SubscriptionsTo() override = default;

    size_t size() const {
      return subscriptions_.size();
    }

    bool empty() const {
      return subscriptions_.empty();
    }

    /// Subscribes
    Subscription subscribe(Callback callback) {
      std::unordered_map<uint64_t, Callback> &m =
          inside_publish_ ? being_subscribed_ : subscriptions_;
      m[++last_ticket_] = std::move(callback);
      return Subscription(last_ticket_, weak_from_this());
    }

    /// Forwards data to subscriptions
    void publish(Args... args) {
      if (empty()) {
        return;
      }

      // N.B. they can subscribe and unsubscribe during callbacks
      inside_publish_ = true;

      for (auto &p : subscriptions_) {
        if (being_canceled_.count(p.first) == 0) {
          // args are not moved of course
          if (filter(p.first, args...)) {
            p.second(args...);
          }
        }
      }

      inside_publish_ = false;

      // maybe someone unsubscribed inside callbacks
      for (auto &ticket : being_canceled_) {
        subscriptions_.erase(ticket);
      }
      being_canceled_.clear();

      // and maybe someone subscribed inside callbacks
      for (auto& [ticket, cb] : being_subscribed_) {
        subscriptions_[ticket] = std::move(cb);
      }
      being_subscribed_.clear();
    }

   protected:

    /// To be overrided, returns true if args applicable to ticket
    virtual bool filter(uint64_t ticket, Args... args) = 0;

    /// Used by derived classes to make filters
    uint64_t lastTicket() { return last_ticket_; }

    void unsubscribe(uint64_t ticket) override {
      if (inside_publish_) {
        being_canceled_.emplace(ticket);
      } else {
        subscriptions_.erase(ticket);
      }
    }

   private:

    uint64_t last_ticket_ = 0;
    std::unordered_map<uint64_t, Callback> subscriptions_;
    std::unordered_map<uint64_t, Callback> being_subscribed_;
    std::unordered_set<uint64_t> being_canceled_;
    bool inside_publish_ = false;
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_SUBSCRIPTIONS_HPP
