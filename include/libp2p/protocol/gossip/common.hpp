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

  /// Time may be any monotonic counter
  using Time = uint64_t;

  // TODO(artem): move to gsl::span+shared_ptr<const uint8_t[]>
  using Bytes = std::vector<uint8_t>;

  template <typename T>
  using Optional = boost::optional<T>;

  template <typename T>
  using Repeated = std::vector<T>;

  using TopicId = std::string;

  using peer::PeerId;

  // message id == string(seq_no + from)
  using MessageId = Bytes;

  /// Message being published
  struct TopicMessage {
    using Ptr = std::shared_ptr<TopicMessage>;

    /// Creates a new message from wire or storage
    static TopicMessage::Ptr fromWire(Bytes _from, Bytes _seq, Bytes _data);

    /// Creates a new message before publishing
    static TopicMessage::Ptr fromScratch(const PeerId& _from, uint64_t _seq, Bytes _data);

    /// Peer id of creator
    const Bytes from;

    /// Sequence number: big endian uint64_t converted to string
    const Bytes seq_no;

    /// Arbitrary data
    const Bytes data;

    /// Topic ids
    Repeated<TopicId> topic_ids;

    // TODO(artem): signing and protobuf issue. Seems they didn't try their
    // kitchen
    Optional<Bytes> signature;
    Optional<Bytes> key;

   protected:
    TopicMessage(Bytes _from, Bytes _seq, Bytes _data);
  };

  /// Returns "zero" peer id, needed for consistency purposes
  const PeerId &getEmptyPeer();

  /// Needed for sets and maps
  inline bool less(const PeerId &a, const PeerId &b) {
    // N.B. toVector returns const std::vector&, i.e. it is fast
    return a.toVector() < b.toVector();
  }

  /// Tries to cast from message field to peer id
  outcome::result<PeerId> peerFrom(const TopicMessage &msg);

  /// Creates seq number byte representation as per pub-sub spec
  Bytes createSeqNo(uint64_t seq);

  /// Helper for text messages creation and protobuf
  Bytes fromString(const std::string &s);

  /// Creates message id as per pub-sub spec
  MessageId createMessageId(const TopicMessage &msg);

  /// Uniform random generator interface
  class UniformRandomGen {
   public:
    /// Creates default impl: MT19937
    static std::shared_ptr<UniformRandomGen> createDefault();

    virtual ~UniformRandomGen() = default;

    /// Returns random size_t in range [0, n]. N.B. n is included!
    virtual size_t operator()(size_t n) = 0;
  };

}  // namespace libp2p::protocol::gossip

namespace std {
  template <>
  struct hash<libp2p::protocol::gossip::Bytes> {
    size_t operator()(const libp2p::protocol::gossip::Bytes &x) const;
  };
}  // namespace std

#endif  // LIBP2P_PROTOCOL_GOSSIP_COMMON_HPP
