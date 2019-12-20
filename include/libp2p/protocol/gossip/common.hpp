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

#include <libp2p/common/byteutil.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace libp2p::protocol::gossip {

  /// Time may be any monotonic counter
  using Time = uint64_t;

  // TODO(artem): move to gsl::span+shared_ptr<const uint8_t[]>
  using common::ByteArray;

  template <typename T>
  using Repeated = std::vector<T>;

  using TopicId = std::string;

  // message id == string(seq_no + from)
  using MessageId = ByteArray;

  /// Message being published
  struct TopicMessage {
    using Ptr = std::shared_ptr<TopicMessage>;

    /// Creates a new message from wire or storage
    static TopicMessage::Ptr fromWire(ByteArray _from, ByteArray _seq, ByteArray _data);

    /// Creates a new message before publishing
    static TopicMessage::Ptr fromScratch(const peer::PeerId& _from, uint64_t _seq, ByteArray _data);

    /// Peer id of creator
    const ByteArray from;

    /// Sequence number: big endian uint64_t converted to string
    const ByteArray seq_no;

    /// Arbitrary data
    const ByteArray data;

    /// Topic ids
    Repeated<TopicId> topic_ids;

    // TODO(artem): signing and protobuf issue. Seems they didn't try their
    // kitchen
    boost::optional<ByteArray> signature;
    boost::optional<ByteArray> key;

   protected:
    TopicMessage(ByteArray _from, ByteArray _seq, ByteArray _data);
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
  ByteArray createSeqNo(uint64_t seq);

  /// Helper for text messages creation and protobuf
  ByteArray fromString(const std::string &s);

  /// Creates message id as per pub-sub spec
  MessageId createMessageId(const TopicMessage &msg);

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_COMMON_HPP
