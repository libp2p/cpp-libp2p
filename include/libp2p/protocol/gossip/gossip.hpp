/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GOSSIP_HPP
#define LIBP2P_GOSSIP_HPP

#include <functional>
#include <set>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/protocol/common/subscription.hpp>

namespace libp2p::protocol::gossip {

  /// Gossip pub-sub protocol config
  struct Config {
    /// Network density factor for gossip meshes
    size_t D = 6;

    /// Ideal number of connected peers to support the network
    size_t ideal_connections_num = 100;

    /// Maximum number of simultaneous connections after which new
    /// incoming peers will be rejected
    size_t max_connections_num = 1000;

    /// Forward messages to all subscribers not in mesh
    /// (floodsub mode compatibility)
    bool floodsub_forward_mode = false;

    /// Forward local message to local subscribers
    bool echo_forward_mode = false;

    /// Read or write timeout per whole network operation
    unsigned rw_timeout_msec = 10000;

    unsigned message_cache_lifetime_msec = 120000;

    unsigned seen_cache_lifetime_msec = 60000;

    unsigned heartbeat_interval_msec = 1000;

    /// Max RPC message size
    size_t max_message_size = 1 << 24;

    /// Protocol version
    std::string protocol_version = "/meshsub/1.0.0";
  };

  using common::ByteArray;

  using TopicId = std::string;
  using TopicList = std::vector<TopicId>;
  using TopicSet = std::set<TopicId>;

  /// Gossip protocol interface
  class Gossip {
   public:
    virtual ~Gossip() = default;

    /// Starts client and server
    virtual void start() = 0;

    /// Stops client and server
    virtual void stop() = 0;

    /// Message received on subscription.
    /// Temporary struct of fields the subscriber may store if they want
    struct Message {
      const ByteArray &from;
      const TopicList &topics;
      const ByteArray &data;
    };

    /// Empty message means EOS (end of subscription data stream)
    using SubscriptionData = boost::optional<const Message &>;
    using SubscriptionCallback = std::function<void(SubscriptionData)>;

    /// Subscribes to topics
    virtual Subscription subscribe(TopicSet topics,
                                   SubscriptionCallback callback) = 0;

    /// Publishes to topics. Returns false if validation fails or not started
    virtual bool publish(const TopicSet &topic, ByteArray data) = 0;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_GOSSIP_HPP
