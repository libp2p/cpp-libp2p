/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/peer_info_with_distance.hpp>

namespace libp2p::protocol::kademlia {

  PeerInfoWithDistance::PeerInfoWithDistance(const PeerInfo &peer_info,
                                             const ContentId& target)
      : peer_info_(peer_info) {
    auto &p1 = peer_info.id.toVector();
    auto &p2 = target.data;
    for (size_t i = 0u; i < distance_.size(); ++i) {
      distance_[i] = p1[i] ^ p2[i];
    }
  }

  bool PeerInfoWithDistance::operator<(const PeerInfoWithDistance &other) const
      noexcept {
    return std::memcmp(distance_.data(), other.distance_.data(),
                       distance_.size())
        < 0;
  }
}  // namespace libp2p::protocol::kademlia
