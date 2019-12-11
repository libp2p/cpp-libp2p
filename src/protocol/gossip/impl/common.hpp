/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_COMMON_HPP
#define LIBP2P_PROTOCOL_GOSSIP_COMMON_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <libp2p/peer/peer_id.hpp>

namespace libp2p::protocol::gossip {

  using Bytes = std::vector<uint8_t>;

  template <typename T> using Optional = boost::optional<T>;

  template <typename T> using Repeated = std::vector<T>;

  using TopicId = std::string;

  using peer::PeerId;

  // message id == string(seq_no + from)
  using MessageId = Bytes;

  /// Message being published
  struct TopicMessage {
    using Ptr = std::shared_ptr<const TopicMessage>;

    /// Peer id of creator
    Bytes from;

    /// Arbitrary data
    Bytes data;

    /// Sequence number: big endian uint64_t converted to string
    Bytes seq_no;

    /// Topic ids
    Repeated<TopicId> topic_ids;

    // TODO(artem): signing and protobuf issue
    Optional<Bytes> signature;
    Optional<Bytes> key;
  };

  /// Returns "zero" peer id, needed for consistency purposes
  const PeerId &getEmptyPeer();

  /// Needed for sets and maps
  inline bool less(const PeerId &a, const PeerId &b) {
    // N.B. toVector returns const std::vector&, i.e. it is fast
    return a.toVector() < b.toVector();
  }

  /// Tries to cast from message field to peer id
  outcome::result<PeerId> peerFrom(const TopicMessage& msg);

  /// Creates seq number byte representation as per pub-sub spec
  Bytes createSeqNo(uint64_t seq);

  /// Creates message id as per pub-sub spec
  MessageId createMessageId(const TopicMessage& msg);

  /// Uniform random generator interface
  class UniformRandomGen {
   public:
    /// Creates default impl: MT19937
    static std::shared_ptr<UniformRandomGen> createDefault();

    virtual ~UniformRandomGen() = default;

    /// Returns random size_t in range [0, n)
    virtual size_t operator()(size_t n) = 0;
  };

} //namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_COMMON_HPP
