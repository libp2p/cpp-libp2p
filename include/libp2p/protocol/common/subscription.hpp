/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_SUBSCRIPTION_HPP
#define LIBP2P_PROTOCOL_SUBSCRIPTION_HPP

#include <cstdint>
#include <memory>

namespace libp2p::protocol {

  /// Lifetime-aware subscription handle
  class Subscription {
   public:
    /// Source of data stream
    struct Source : public std::enable_shared_from_this<Source> {
      virtual ~Source() = default;
      virtual void unsubscribe(uint64_t ticket) = 0;
    };

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = default;

    Subscription();
    Subscription& operator=(Subscription&&) noexcept;
    Subscription(uint64_t ticket, std::weak_ptr<Source> source);
    ~Subscription();

    void cancel();

   private:
    uint64_t ticket_;
    std::weak_ptr<Source> source_wptr_;
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_SUBSCRIPTION_HPP
