/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/protocol.hpp>
#include <libp2p/protocol/common/subscription.hpp>
#include <libp2p/protocol/gossip/peer_kind.hpp>
#include <libp2p/protocol/gossip/score_config.hpp>

namespace libp2p {
  struct Host;
  namespace basic {
    class Scheduler;
  }
  namespace crypto {
    class CryptoProvider;
    namespace marshaller {
      class KeyMarshaller;
    }
  }  // namespace crypto
  namespace peer {
    struct IdentityManager;
  }
}  // namespace libp2p

namespace libp2p::protocol::gossip {

  /// Gossip pub-sub protocol config
  struct Config {
    /// Network density factors for gossip meshes
    size_t D_min = 5;
    size_t D = 6;
    size_t D_max = 10;

    /// Ideal number of connected peers to support the network
    size_t ideal_connections_num = 100;

    /// Maximum number of simultaneous connections after which new
    /// incoming peers will be rejected
    size_t max_connections_num = 1000;

    /// Forward local message to local subscribers
    bool echo_forward_mode = false;

    /// Read or write timeout per whole network operation
    std::chrono::milliseconds rw_timeout_msec{std::chrono::seconds(10)};

    /// Heartbeat interval
    std::chrono::milliseconds heartbeat_interval_msec{1000};

    /// Ban interval between dial attempts to peer
    std::chrono::milliseconds ban_interval_msec{std::chrono::minutes(1)};

    /// Max number of dial attempts before peer is forgotten
    unsigned max_dial_attempts = 3;

    /// Expiration of gossip peers' addresses in address repository
    std::chrono::milliseconds address_expiration_msec{std::chrono::hours(1)};

    /// Max RPC message size
    size_t max_message_size = 1 << 24;

    /// Protocol versions
    std::unordered_map<ProtocolName, PeerKind> protocol_versions{
        {"/floodsub/1.0.0", PeerKind::Floodsub},
        {"/meshsub/1.0.0", PeerKind::Gossipsub},
        {"/meshsub/1.1.0", PeerKind::Gossipsubv1_1},
        {"/meshsub/1.2.0", PeerKind::Gossipsubv1_2},
    };

    /// Sign published messages
    bool sign_messages = false;

    size_t history_length{5};

    size_t history_gossip{3};

    std::chrono::seconds fanout_ttl{60};

    std::chrono::seconds duplicate_cache_time{60};

    std::chrono::seconds prune_backoff{60};

    std::chrono::seconds unsubscribe_backoff{10};

    size_t backoff_slack = 1;

    bool flood_publish = true;

    size_t max_ihave_length = 5000;

    ScoreConfig score;
  };

  using TopicId = std::string;
  using TopicList = std::vector<TopicId>;
  using TopicSet = std::set<TopicId>;

  /// Gossip protocol interface
  class Gossip {
   public:
    virtual ~Gossip() = default;

    /// Adds bootstrap peer to the set of connectable peers
    virtual void addBootstrapPeer(
        const peer::PeerId &id,
        boost::optional<multi::Multiaddress> address) = 0;

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
      const Bytes &from;
      const TopicId &topic;
      const Bytes &data;
    };

    /// Validator of messages arriving from the wire
    using Validator = std::function<bool(const Bytes &from, const Bytes &data)>;

    /// Sets message validator for topic
    virtual void setValidator(const TopicId &topic, Validator validator) = 0;

    /// Creates unique message ID out of message fields
    using MessageIdFn = std::function<Bytes(
        const Bytes &from, const Bytes &seq, const Bytes &data)>;

    /// Sets message ID funtion that differs from default (from+sec_no)
    virtual void setMessageIdFn(MessageIdFn fn) = 0;

    /// Empty message means EOS (end of subscription data stream)
    using SubscriptionData = boost::optional<const Message &>;
    using SubscriptionCallback = std::function<void(SubscriptionData)>;

    /// Subscribes to topics
    virtual Subscription subscribe(TopicSet topics,
                                   SubscriptionCallback callback) = 0;

    /// Publishes to topics. Returns false if validation fails or not started
    virtual bool publish(TopicId topic, Bytes data) = 0;
  };

  // Creates Gossip object
  std::shared_ptr<Gossip> create(
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<Host> host,
      std::shared_ptr<peer::IdentityManager> idmgr,
      std::shared_ptr<crypto::CryptoProvider> crypto_provider,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      Config config = Config{});

}  // namespace libp2p::protocol::gossip
