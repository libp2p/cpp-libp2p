/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/common.hpp>

#include <random>

#include <boost/container_hash/hash.hpp>
#include <boost/endian/conversion.hpp>

namespace libp2p::protocol::gossip {

  namespace {
    PeerId createEmptyPeer() {
      static const size_t kHashSize = 32;
      // hash that belongs to no-one
      std::array<uint8_t, kHashSize> generic_hash{};
      generic_hash.fill(0);
      auto h = multi::Multihash::create(multi::sha256, generic_hash);
      assert(h);
      auto p = PeerId::fromHash(h.value());
      assert(p);
      return p.value();
    }

    class MTRandom : public UniformRandomGen {
      using Distr = std::uniform_int_distribution<size_t>;

     public:
      MTRandom() : eng_(rd_()) {}

     private:
      size_t operator()(size_t n) override {
        return distr_(eng_, Distr::param_type(0, n));
      }

      std::random_device rd_;
      std::mt19937_64 eng_;
      Distr distr_;
    };

    template <class T>
    struct SPtrHack : public T {
      template <typename... Args>
      explicit SPtrHack(Args... args) : T(std::forward<Args>(args)...) {}
    };

  }  // namespace

  const PeerId &getEmptyPeer() {
    static const PeerId peer(createEmptyPeer());
    return peer;
  }

  outcome::result<PeerId> peerFrom(const TopicMessage &msg) {
    return PeerId::fromBytes(msg.from);
  }

  Bytes createSeqNo(uint64_t seq) {
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
    return Bytes(seq_buf.bytes.begin(), seq_buf.bytes.end());
  }

  Bytes fromString(const std::string &s) {
    Bytes ret{};
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

  TopicMessage::TopicMessage(Bytes _from, Bytes _seq, Bytes _data)
      : from(std::move(_from)),
        seq_no(std::move(_seq)),
        data(std::move(_data)) {}

  TopicMessage::Ptr TopicMessage::fromWire(Bytes _from, Bytes _seq,
                                           Bytes _data) {
    return std::make_shared<SPtrHack<TopicMessage>>(
        std::move(_from), std::move(_seq), std::move(_data));
  }

  TopicMessage::Ptr TopicMessage::fromScratch(const PeerId& _from, uint64_t _seq,
                                           Bytes _data) {
    return fromWire(_from.toVector(), createSeqNo(_seq), std::move(_data));
  }

  std::shared_ptr<UniformRandomGen> UniformRandomGen::createDefault() {
    return std::make_shared<MTRandom>();
  }

}  // namespace libp2p::protocol::gossip

size_t std::hash<libp2p::protocol::gossip::Bytes>::operator()(
    const libp2p::protocol::gossip::Bytes &x) const {
  return boost::hash_range(x.begin(), x.end());
}
