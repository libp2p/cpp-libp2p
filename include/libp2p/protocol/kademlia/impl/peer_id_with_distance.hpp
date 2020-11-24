/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_PEERINFOWITHDISTANCE
#define LIBP2P_PROTOCOL_KADEMLIA_PEERINFOWITHDISTANCE

#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>

namespace libp2p::protocol::kademlia {

  struct PeerIdWithDistance {

    template <typename T>
    PeerIdWithDistance(const PeerId &peer_id, T &&target)
        : peer_id_(peer_id),
          distance_(NodeId(peer_id).distance(NodeId(std::forward<T>(target)))) {
    }

    bool operator<(const PeerIdWithDistance &other) const noexcept {
      return std::memcmp(distance_.data(), other.distance_.data(),
                         distance_.size())
          < 0;
    }

    const PeerId &operator*() const {
      return peer_id_.get();
    }

    explicit operator PeerId() const {
      return peer_id_.get();
    }

    std::reference_wrapper<const PeerId> peer_id_;
    common::Hash256 distance_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_PEERINFOWITHDISTANCE
