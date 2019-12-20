/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/common.hpp>

#include <random>

#include <boost/endian/conversion.hpp>

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

    template <class T>
    struct SPtrHack : public T {
      template <typename... Args>
      explicit SPtrHack(Args... args) : T(std::forward<Args>(args)...) {}
    };

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

  MessageId createMessageId(const TopicMessage &msg) {
    MessageId msg_id(msg.seq_no);
    msg_id.reserve(msg.seq_no.size() + msg.from.size());
    msg_id.insert(msg_id.end(), msg.from.begin(), msg.from.end());
    return msg_id;
  }

  TopicMessage::TopicMessage(ByteArray _from, ByteArray _seq, ByteArray _data)
      : from(std::move(_from)),
        seq_no(std::move(_seq)),
        data(std::move(_data)) {}

  TopicMessage::Ptr TopicMessage::fromWire(ByteArray _from, ByteArray _seq,
                                           ByteArray _data) {
    return std::make_shared<SPtrHack<TopicMessage>>(
        std::move(_from), std::move(_seq), std::move(_data));
  }

  TopicMessage::Ptr TopicMessage::fromScratch(const peer::PeerId& _from, uint64_t _seq,
                                              ByteArray _data) {
    return fromWire(_from.toVector(), createSeqNo(_seq), std::move(_data));
  }

}  // namespace libp2p::protocol::gossip
