/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_PEER_CONTEXT_HPP
#define LIBP2P_PROTOCOL_GOSSIP_PEER_CONTEXT_HPP

#include <libp2p/common/metrics/instance_count.hpp>

#include "common.hpp"

namespace libp2p::protocol::gossip {

  class MessageBuilder;
  class Stream;

  /// Data related to peer needed by pub-sub protocols
  struct PeerContext {
    /// The key
    const peer::PeerId peer_id;

    /// String repr for logging purposes
    const std::string str;

    /// Builds message to be sent to this peer
    std::shared_ptr<MessageBuilder> message_builder;

    /// Set of topics this peer is subscribed to
    std::set<TopicId> subscribed_to;

    /// Streams connected to peer
    std::shared_ptr<Stream> outbound_stream;
    std::vector<std::shared_ptr<Stream>> inbound_streams;

    /// Dialing to this peer is banned until this timestamp
    Time banned_until{0};

    /// Current dial attempts
    unsigned dial_attempts = 0;

    /// If true, then outbound connection is in progress
    bool is_connecting = false;

    ~PeerContext() = default;
    PeerContext(PeerContext &&) = delete;
    PeerContext(const PeerContext &) = delete;
    PeerContext &operator=(const PeerContext &) = delete;
    PeerContext &operator=(PeerContext &&) = delete;

    explicit PeerContext(peer::PeerId id);

    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(
        libp2p::protocol::gossip::PeerContext);
  };

  /// Operators needed to place PeerContextPtr into PeerSet but use
  /// const peer::PeerId& as a key
  bool operator<(const PeerContextPtr &ctx, const peer::PeerId &peer);
  bool operator<(const peer::PeerId &peer, const PeerContextPtr &ctx);
  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b);

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_PEER_CONTEXT_HPP
