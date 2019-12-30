/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_PEER_CONTEXT_HPP
#define LIBP2P_PROTOCOL_GOSSIP_PEER_CONTEXT_HPP

#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/protocol/gossip/impl/common.hpp>

namespace libp2p::protocol::gossip {

  class MessageBuilder;
  class StreamWriter;
  class StreamReader;

  /// Data related to peer needed by pub-sub protocols
  struct PeerContext {
    /// The key
    const peer::PeerId peer_id;

    /// Set of topics this peer is subscribed to
    std::set<TopicId> subscribed_to;

    /// Builds message to be sent to this peer
    std::shared_ptr<MessageBuilder> message_to_send;

    /// Network stream writer
    std::shared_ptr<StreamWriter> writer;

    /// Network stream reader
    std::shared_ptr<StreamReader> reader;

    /// Not null iff this peer can be dialed to
    boost::optional<multi::Multiaddress> dial_to;

    /// Dialing to this peer is banned until this timestamp
    Time banned_until = 0;

    ~PeerContext() = default;
    PeerContext(PeerContext &&) = delete;
    PeerContext(const PeerContext &) = delete;
    PeerContext &operator=(const PeerContext &) = delete;
    PeerContext &operator=(PeerContext &&) = delete;

    explicit PeerContext(peer::PeerId id) : peer_id(std::move(id)) {}
  };

  /// Operators needed to place PeerContextPtr into PeerSet but use
  /// const peer::PeerId& as a key
  bool operator<(const PeerContextPtr &ctx, const peer::PeerId &peer);
  bool operator<(const peer::PeerId &peer, const PeerContextPtr &ctx);
  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b);

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_PEER_CONTEXT_HPP
