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
  }  // namespace event

  // TODO(warchant): when connection is closed ('onDisconnected' event fired),
  // manager should remove it from storage PRE-212

  /**
   * @brief Connection Manager stores all known connections, and is capable of
   * selecting subset of connections
   */
  struct ConnectionManager : public basic::GarbageCollectable {
    using Connection = connection::CapableConnection;
    using ConnectionSPtr = std::shared_ptr<Connection>;

    enum class Connectedness {
      NOT_CONNECTED,  ///< we don't know peer's addresses, and are not connected
      CONNECTED,      ///< we have at least one connection to this peer
      CAN_CONNECT,    ///< we know peer's addr, and we can dial
      CAN_NOT_CONNECT  ///< we know peer's addr, but can not dial (no
                       ///< transports)
    };

    ~ConnectionManager() override = default;

    // get list of all connections (including inbound and outbound)
    virtual std::vector<ConnectionSPtr> getConnections() const = 0;

    // get list of all inbound or outbound connections to a given peer.
    virtual std::vector<ConnectionSPtr> getConnectionsToPeer(
        const peer::PeerId &p) const = 0;

    // get best connection to a given peer
    virtual ConnectionSPtr getBestConnectionForPeer(
        const peer::PeerId &p) const = 0;

    // get connectedness information for given peer p
    virtual Connectedness connectedness(const peer::PeerInfo &p) const = 0;

    // add connection to a given peer
    virtual void addConnectionToPeer(const peer::PeerId &p,
                                     ConnectionSPtr c) = 0;

    // closes all connections (outbound and inbound) to given peer
    virtual void closeConnectionsToPeer(const peer::PeerId &p) = 0;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_CONNECTION_MANAGER_HPP
