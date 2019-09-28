/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PEER_ROUTING_HPP
#define LIBP2P_PEER_ROUTING_HPP

#include <libp2p/peer/peer_info.hpp>

namespace libp2p::protocol::kademlia {

  /// PeerRouting is a way to find information about certain peers.
  ///
  /// This can be implemented by a simple lookup table, a tracking server,
  /// or even a DHT (like herein).
  struct PeerRouting {
    virtual ~PeerRouting() = default;

    using FindPeerResult = outcome::result<peer::PeerInfo>;
    using FindPeerResultFunc = std::function<void(FindPeerResult)>;

    /// FindPeer searches for a peer with given ID, returns a PeerInfo with
    /// relevant addresses.
    virtual void findPeer(peer::PeerId id, FindPeerResultFunc f) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PEER_ROUTING_HPP
