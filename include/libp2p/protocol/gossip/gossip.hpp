/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GOSSIP_HPP
#define LIBP2P_GOSSIP_HPP

#include <chrono>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/common/subscription.hpp>

namespace libp2p {
  struct Host;
  namespace basic {
    class Scheduler;
  }
}  // namespace libp2p

namespace libp2p::protocol::gossip {

  /// Gossip pub-sub protocol config
  struct Config {
    /// Network density factors for gossip meshes
    size_t D_min = 5;
    size_t D_max = 10;

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
    std::chrono::milliseconds rw_timeout_msec {10000};

    /// Lifetime of a message in message cache
    std::chrono::milliseconds message_cache_lifetime_msec { 120000 };

    std::chrono::milliseconds seen_cache_lifetime_msec { 60000 };

    std::chrono::milliseconds heartbeat_interval_msec { 1000 };

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

    /// Adds bootstrap peer to the set of connectable peers
    virtual void addBootstrapPeer(
        peer::PeerId id, boost::optional<multi::Multiaddress> address) = 0;

    /// Adds bootstrap peer address in string form
    virtual outcome::result<void> addBootstrapPeer(
        const std::string &address) = 0;

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

    /// Validator of messages arriving from the wire
    using Validator =
        std::function<bool(const ByteArray &from, const ByteArray &data)>;

    /// Sets message validator for topic
    virtual void setValidator(const TopicId &topic, Validator validator) = 0;

    /// Creates unique message ID out of message fields
    using MessageIdFn = std::function<ByteArray(
        const ByteArray &from, const ByteArray &seq, const ByteArray &data)>;

    /// Sets message ID funtion that differs from default (from+sec_no)
    virtual void setMessageIdFn(MessageIdFn fn) = 0;

    /// Empty message means EOS (end of subscription data stream)
    using SubscriptionData = boost::optional<const Message &>;
    using SubscriptionCallback = std::function<void(SubscriptionData)>;

    /// Subscribes to topics
    virtual Subscription subscribe(TopicSet topics,
                                   SubscriptionCallback callback) = 0;

    /// Publishes to topics. Returns false if validation fails or not started
    virtual bool publish(const TopicSet &topic, ByteArray data) = 0;
  };

  // Creates Gossip object
  std::shared_ptr<Gossip> create(std::shared_ptr<basic::Scheduler> scheduler,
                                 std::shared_ptr<Host> host,
                                 Config config = Config{});

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_GOSSIP_HPP
