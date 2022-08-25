/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_NODEID
#define LIBP2P_PROTOCOL_KADEMLIA_NODEID

#include <bitset>
#include <climits>
#include <cstring>
#include <gsl/span>
#include <memory>
#include <vector>

#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/content_id.hpp>

namespace libp2p::protocol::kademlia {

  using common::Hash256;
  using crypto::sha256;

  /// count number of leading zeros in bin representation
  inline size_t leadingZerosInByte(uint8_t byte) {
    size_t ret = 0u;
    static constexpr const uint8_t msb = 1u << 7u;

    // if most significant bit is set, terminate the loop
    while ((byte & msb) == 0) {
      ret++;
      byte <<= 1u;
    }

    return ret;
  }

  inline auto xor_distance(const Hash256 &a, const Hash256 &b) {
    Hash256 x0r = a;
    constexpr auto size = Hash256().size();
    // calculate XOR in-place
    for (size_t i = 0u; i < size; ++i) {
      x0r[i] ^= b[i];
    }

    return x0r;
  }

  /**
   * @brief DHT Node ID implementation
   */
  class NodeId {
   public:
    explicit NodeId(const Hash256 &h) : data_(h) {}

    explicit NodeId(const void *bytes) {
      memcpy(data_.data(), bytes, data_.size());
    }

    explicit NodeId(const peer::PeerId &pid) {
      auto digest_res = crypto::sha256(pid.toVector());
      BOOST_ASSERT(digest_res.has_value());
      data_ = std::move(digest_res.value());
    }

    explicit NodeId(const ContentId &content_id) {
      auto digest_res = crypto::sha256(content_id);
      BOOST_ASSERT(digest_res.has_value());
      data_ = std::move(digest_res.value());
    }

    inline bool operator==(const NodeId &other) const {
      return data_ == other.data_;
    }

    inline bool operator==(const Hash256 &h) const {
      return data_ == h;
    }

    inline Hash256 distance(const NodeId &other) const {
      return xor_distance(data_, other.data_);
    }

    /// number of common prefix bits between this and NodeId
    inline size_t commonPrefixLen(const NodeId &other) const {
      constexpr int uint8_bits = CHAR_BIT;
      constexpr auto size = Hash256().size();
      Hash256 d = distance(other);
      for (size_t i = 0; i < size; ++i) {
        if (d[i] != 0) {
          return i * uint8_bits + leadingZerosInByte(d[i]);
        }
      }

      return size * uint8_bits;
    }

    inline const Hash256 &getData() const {
      return data_;
    }

    inline Hash256 &getData() {
      return data_;
    }

   private:
    Hash256 data_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_NODEID
