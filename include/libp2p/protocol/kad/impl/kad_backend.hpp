/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_BACKEND_HPP
#define LIBP2P_KAD_BACKEND_HPP

#include <libp2p/protocol/kad/common.hpp>
#include <libp2p/protocol/kad/node_id.hpp>

namespace libp2p::protocol::kad {

  /// Backend for feedback from internal Kademlia components
  class KadBackend {
   public:
    virtual ~KadBackend() = default;

    virtual const struct KademliaConfig &config() = 0;

    virtual class Scheduler &scheduler() = 0;

    /// broadcasts ADD_PROVIDER request to appropriate nodes
    virtual void broadcastThisProvider(const ContentAddress& key) {
      // stub by default
    }

    virtual PeerIdVec getNearestPeers(const NodeId& id) {
      // stub by default
      return PeerIdVec();
    }
  };

}  // namespace libp2p::protocol::kad

#endif  // LIBP2P_KAD_BACKEND_HPP
