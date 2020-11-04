/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_PEERINFOWITHDISTANCE
#define LIBP2P_PROTOCOL_KADEMLIA_PEERINFOWITHDISTANCE

#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  struct PeerInfoWithDistance {
    PeerInfoWithDistance(const PeerInfo &peer_info, const ContentId& target);

    bool operator<(const PeerInfoWithDistance &other) const noexcept;

    const PeerInfo &operator*() const {
      return peer_info_.get();
    }

    std::reference_wrapper<const PeerInfo> peer_info_;
    common::Hash256 distance_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_PEERINFOWITHDISTANCE
