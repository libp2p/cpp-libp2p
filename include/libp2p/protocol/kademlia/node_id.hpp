/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_NODE_ID_HPP
#define LIBP2P_KAD_NODE_ID_HPP

#include <bitset>
#include <vector>
#include <memory>
#include <cstring>


#include <gsl/span>
#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/peer/peer_id.hpp>

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

    explicit NodeId(const peer::PeerId &pid) : data_(sha256(pid.toVector())) {}

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
      constexpr int uint8_bits = 8;
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

  struct XorDistanceComparator {
    explicit XorDistanceComparator(const peer::PeerId &from) {
      hfrom = sha256(from.toVector());
    }

    explicit XorDistanceComparator(const NodeId &from)
        : hfrom(from.getData()) {}

    explicit XorDistanceComparator(const Hash256 &hash) : hfrom(hash) {}

    bool operator()(const peer::PeerId &a, const peer::PeerId &b) {
      NodeId from(hfrom);
      auto d1 = NodeId(a).distance(from);
      auto d2 = NodeId(b).distance(from);
      constexpr auto size = Hash256().size();

      // return true, if distance d1 is less than d2, false otherwise
      return std::memcmp(d1.data(), d2.data(), size) < 0;
    }

    Hash256 hfrom;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_NODE_ID_HPP
