/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_MANAGER_HPP
#define LIBP2P_CONNECTION_MANAGER_HPP

#include <memory>

#include <libp2p/basic/garbage_collectable.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/peer/peer_info.hpp>

namespace libp2p::network {

  namespace event {
    /// fired when any new connection, in or outbound, is created
    struct OnNewConnection {};
    using OnNewConnectionChannel = libp2p::event::channel_decl<
        OnNewConnection, std::weak_ptr<connection::CapableConnection>>;

    /// fired when all connections to peer closed
    struct PeerDisconnected {};
    using OnPeerDisconnectedChannel =
        libp2p::event::channel_decl<PeerDisconnected, const peer::PeerId &>;
  }  // namespace event

  /**
   * @brief Connection Manager stores all known connections, and is capable of
   * selecting subset of connections
   */
  struct ConnectionManager : public basic::GarbageCollectable {
    using Connection = connection::CapableConnection;
    using ConnectionSPtr = std::shared_ptr<Connection>;

    ~ConnectionManager() override = default;

    // get list of all connections (including inbound and outbound)
    virtual std::vector<ConnectionSPtr> getConnections() const = 0;

    // get list of all inbound or outbound connections to a given peer.
    virtual std::vector<ConnectionSPtr> getConnectionsToPeer(
        const peer::PeerId &p) const = 0;

    // get best connection to a given peer
    virtual ConnectionSPtr getBestConnectionForPeer(
        const peer::PeerId &p) const = 0;

    // add connection to a given peer
    virtual void addConnectionToPeer(const peer::PeerId &p,
                                     ConnectionSPtr c) = 0;

    // closes all connections (outbound and inbound) to given peer
    virtual void closeConnectionsToPeer(const peer::PeerId &p) = 0;

    // called from connections when they are closed
    // TODO(artem) connection IDs instead of indexing by sptr
    virtual void onConnectionClosed(
        const peer::PeerId &peer_id,
        const std::shared_ptr<connection::CapableConnection> &conn) = 0;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_CONNECTION_MANAGER_HPP
