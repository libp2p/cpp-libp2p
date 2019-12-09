/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GOSSIP_WIRE_PROTOCOL_HPP
#define LIBP2P_GOSSIP_WIRE_PROTOCOL_HPP

#include <cstdint>
#include <vector>
#include <string>

#include <gsl/span>
#include <boost/optional.hpp>

namespace libp2p::protocol::gossip {

  using Bytes = std::vector<uint8_t>;
  template <typename T> using Optional = boost::optional<T>;
  template <typename T> using Repeated = std::vector<T>;
  using TopicId = std::string;

  // TODO(artem): check if message id == from + seq_no (???)
  using MessageId = std::string;

  /// Subscribe-unsubscribe request
  struct SubOpts {
    /// true means subscribe, false means unsubscribe
    bool subscribe = false;

    /// Pub-sub topic
    TopicId topic_id;
  };

  /// Message being published
  struct TopicMessage {
    // TODO(artem): peerId or whatever?
    Bytes from;

    Bytes data;

    // TODO(artem): why sequence number is not integer?
    Bytes seq_no;

    Repeated<TopicId> topic_ids;

    Optional<Bytes> signature;
    Optional<Bytes> key;
  };

  /// Announce about topic messages presented on this host
  struct IHave {
    TopicId topic_id;
    Repeated<MessageId> message_ids;
  };

  /// Pub-sub subnet control message
  struct ControlMessage {
    /// Announces by topics
    Repeated<IHave> i_have;

    /// Requests for messages
    Repeated<MessageId> i_want;

    /// Requests to join per-topic mesh
    Repeated<TopicId> graft;

    /// Requests to leave per-topic mesh
    Repeated<TopicId> prune;

    bool empty() const;

    void clear();
  };

  /// General wire protocol message
  struct GossipMessage {
    /// Subscribe-unsubscribe requests
    Repeated<SubOpts> subscriptions;

    /// Messages to publish
    Repeated<TopicMessage> publish;

    /// Control messages
    Optional<ControlMessage> control;

    bool empty() const;

    void clear();

    bool deserialize(gsl::span<const uint8_t> bytes);

    bool serialize(std::vector<uint8_t>& buffer) const;
  };


  // TODO(artem): TopicDescriptor

} //namespace libp2p::protocol::gossip


#endif  // LIBP2P_GOSSIP_WIRE_PROTOCOL_HPP
