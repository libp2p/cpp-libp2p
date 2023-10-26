/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>

#include <libp2p/protocol/gossip/gossip.hpp>

namespace libp2p::protocol::gossip {

  /// Error codes
  enum class Error {
    MESSAGE_PARSE_ERROR = 1,
    MESSAGE_SIZE_ERROR,
    MESSAGE_SERIALIZE_ERROR,
    MESSAGE_WRITE_ERROR,
    READER_DISCONNECTED,
    WRITER_DISCONNECTED,
    READER_TIMEOUT,
    WRITER_TIMEOUT,
    CANNOT_CONNECT,
    VALIDATION_FAILED
  };

  /// Success indicator to be passed in outcome::result
  struct Success {};

  /// Shared buffer used to broadcast messages
  using SharedBuffer = std::shared_ptr<const Bytes>;

  /// Time is scheduler's clock and counter
  using Time = std::chrono::milliseconds;

  template <typename T>
  using Repeated = std::vector<T>;

  using TopicId = std::string;

  // message id == string(seq_no + from)
  using MessageId = Bytes;

  /// Remote peer and its context
  using PeerContextPtr = std::shared_ptr<struct PeerContext>;

  /// Message being published
  struct TopicMessage {
    using Ptr = std::shared_ptr<TopicMessage>;

    /// Peer id of creator
    const Bytes from;

    /// Sequence number: big endian uint64_t converted to string
    const Bytes seq_no;

    /// Arbitrary data
    const Bytes data;

    /// Topic ids
    TopicId topic;

    // TODO(artem): signing and protobuf issue. Seems they didn't try their
    // kitchen
    boost::optional<Bytes> signature;
    boost::optional<Bytes> key;

    /// Creates a new message from wire or storage
    TopicMessage(Bytes _from, Bytes _seq, Bytes _data);

    /// Creates topic message from scratch before publishing
    TopicMessage(const peer::PeerId &_from,
                 uint64_t _seq,
                 Bytes _data,
                 TopicId _topic);
  };

  /// Returns "zero" peer id, needed for consistency purposes
  const peer::PeerId &getEmptyPeer();

  /// Needed for sets and maps
  inline bool less(const peer::PeerId &a, const peer::PeerId &b) {
    // N.B. toVector returns const std::vector&, i.e. it is fast
    return a.toVector() < b.toVector();
  }

  /// Tries to cast from message field to peer id
  outcome::result<peer::PeerId> peerFrom(const TopicMessage &msg);

  /// Creates seq number byte representation as per pub-sub spec
  Bytes createSeqNo(uint64_t seq);

  /// Helper for text messages creation and protobuf
  Bytes fromString(const std::string &s);

  /// Creates message id, default function
  MessageId createMessageId(const Bytes &from,
                            const Bytes &seq,
                            const Bytes &data);
}  // namespace libp2p::protocol::gossip

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::gossip, Error);
