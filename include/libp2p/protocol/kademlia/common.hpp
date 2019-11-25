/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_COMMON_HPP
#define LIBP2P_KADEMLIA_COMMON_HPP

#include <string>
#include <unordered_set>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>

namespace libp2p::protocol::kademlia {

  using libp2p::common::Hash256;

  /// DHT key
  using Key = std::vector<uint8_t>;

  /// DHT value
  using Value = std::vector<uint8_t>;

  /// Set of peer Ids
  using PeerIdSet = std::unordered_set<peer::PeerId>;

  /// Vector of peer Ids
  using PeerIdVec = std::vector<peer::PeerId>;

  /// Set of peer Infos
  using PeerInfoSet = std::unordered_set<peer::PeerInfo>;

  /// Content Id
  struct Cid {
    // TODO(warchant): tbd
  };

  struct ReceivedValue {
    Value value;
    peer::PeerId from;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KADEMLIA_COMMON_HPP
