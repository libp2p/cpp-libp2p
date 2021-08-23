/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.hpp"

#include <boost/endian/conversion.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol::gossip, Error, e) {
  using E = libp2p::protocol::gossip::Error;
  switch (e) {
    case E::MESSAGE_PARSE_ERROR:
      return "message parse error";
    case E::MESSAGE_SIZE_ERROR:
      return "message size out of bounds";
    case E::MESSAGE_SERIALIZE_ERROR:
      return "message serialize error";
    case E::MESSAGE_WRITE_ERROR:
      return "message write error";
    case E::READER_DISCONNECTED:
      return "stream reader disconnected";
    case E::WRITER_DISCONNECTED:
      return "stream writer disconnected";
    case E::READER_TIMEOUT:
      return "stream reader timeout";
    case E::WRITER_TIMEOUT:
      return "stream writer disconnected";
    case E::CANNOT_CONNECT:
      return "cannot connect to peer";
    case E::VALIDATION_FAILED:
      return "validation failed";
    default:
      break;
  }
  return "unknown error";
}

namespace libp2p::protocol::gossip {

  namespace {
    peer::PeerId createEmptyPeer() {
      static const size_t kHashSize = 32;
      // hash that belongs to no-one
      std::array<uint8_t, kHashSize> generic_hash{};
      generic_hash.fill(0);
      auto h = multi::Multihash::create(multi::sha256, generic_hash);
      assert(h);
      auto p = peer::PeerId::fromHash(h.value());
      assert(p);
      return p.value();
    }
  }  // namespace

  const peer::PeerId &getEmptyPeer() {
    static const peer::PeerId peer(createEmptyPeer());
    return peer;
  }

  outcome::result<peer::PeerId> peerFrom(const TopicMessage &msg) {
    return peer::PeerId::fromBytes(msg.from);
  }

  ByteArray createSeqNo(uint64_t seq) {
    // NOLINTNEXTLINE
    union {
      uint64_t number;
      std::array<uint8_t, 8> bytes;
    } seq_buf;

    // NOLINTNEXTLINE
    seq_buf.number = seq;

    // NOLINTNEXTLINE
    boost::endian::native_to_big_inplace(seq_buf.number);

    // NOLINTNEXTLINE
    return ByteArray(seq_buf.bytes.begin(), seq_buf.bytes.end());
  }

  ByteArray fromString(const std::string &s) {
    ByteArray ret{};
    auto sz = s.size();
    if (sz > 0) {
      ret.reserve(sz);
      ret.assign(s.begin(), s.end());
    }
    return ret;
  }

  MessageId createMessageId(const ByteArray &from, const ByteArray &seq,
                            const ByteArray &data) {
    MessageId msg_id(from);
    msg_id.reserve(seq.size() + from.size());
    msg_id.insert(msg_id.end(), seq.begin(), seq.end());
    return msg_id;
  }

  TopicMessage::TopicMessage(ByteArray _from, ByteArray _seq, ByteArray _data)
      : from(std::move(_from)),
        seq_no(std::move(_seq)),
        data(std::move(_data)) {}

  TopicMessage::TopicMessage(const peer::PeerId &_from, uint64_t _seq,
                             ByteArray _data, TopicId _topic)
      : from(_from.toVector()),
        seq_no(createSeqNo(_seq)),
        data(std::move(_data)),
        topic(std::move(_topic)) {}

}  // namespace libp2p::protocol::gossip
