/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_ROUTING_TABLE_HPP
#define LIBP2P_KAD_ROUTING_TABLE_HPP

#include <libp2p/event/bus.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>

namespace libp2p::protocol::kademlia {

  namespace events {
    using PeerAddedChannel =
        event::channel_decl<struct PeerAdded, peer::PeerId>;

    using PeerRemovedChannel =
        event::channel_decl<struct PeerRemoved, peer::PeerId>;

  }  // namespace events

  /**
   * @class RoutingTable
   *
   * Equivalent implementation of RoutingTable from
   * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket
   */
  struct RoutingTable {
    virtual ~RoutingTable() = default;

    /// Update adds or moves the given peer to the front of its respective
    /// bucket
    virtual outcome::result<void> update(const peer::PeerId &pid) = 0;

    /// Remove deletes a peer from the routing table. This is to be used
    /// when we are sure a node has disconnected completely.
    virtual void remove(const peer::PeerId &id) = 0;

    virtual PeerIdVec getAllPeers() const = 0;

    /// NearestPeers returns a list of the 'count' closest peers to the given ID
    virtual PeerIdVec getNearestPeers(const NodeId &id, size_t count) = 0;

    /// Size returns the total number of peers in the routing table
    virtual size_t size() const = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_ROUTING_TABLE_HPP
