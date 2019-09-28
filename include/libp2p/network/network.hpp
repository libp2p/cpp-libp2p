/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_NETWORK_HPP
#define LIBP2P_NETWORK_HPP

#include <libp2p/event/bus.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/dialer.hpp>
#include <libp2p/network/listener_manager.hpp>

namespace libp2p::network {

  /// Network is the interface used to connect to the outside world.
  struct Network {
    virtual ~Network() = default;

    /// Closes all connections to a given peer
    virtual void closeConnections(const peer::PeerId &p) = 0;

    /// Getter for Dialer associated with this Network
    virtual Dialer &getDialer() = 0;

    /// Getter for Listener associated with this Network
    virtual ListenerManager &getListener() = 0;

    /// Getter for Connection Manager associated with this Network
    virtual ConnectionManager &getConnectionManager() = 0;

    // TODO(Warchant): emits events:
    //    ClosedStream(Network, Stream)
    //    Connected(Network, Conn)
    //    Disconnected(Network, Conn)
    //    Listen(Network, Multiaddr)
    //    ListenClose(Network, Multiaddr)
    //    OpenedStream(Network, Stream)
  };

}  // namespace libp2p::network

#endif  // LIBP2P_NETWORK_HPP
