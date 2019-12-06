/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_MANAGER_IMPL_HPP
#define LIBP2P_CONNECTION_MANAGER_IMPL_HPP

#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/transport_manager.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/event/bus.hpp>

namespace libp2p::network {

  class ConnectionManagerImpl : public ConnectionManager {
   public:
    explicit ConnectionManagerImpl(
        std::shared_ptr<libp2p::event::Bus> bus,
        std::shared_ptr<network::TransportManager> tmgr);

    std::vector<ConnectionSPtr> getConnections() const override;

    std::vector<ConnectionSPtr> getConnectionsToPeer(
        const peer::PeerId &p) const override;

    ConnectionSPtr getBestConnectionForPeer(
        const peer::PeerId &p) const override;

    Connectedness connectedness(const peer::PeerInfo &p) const override;

    void addConnectionToPeer(const peer::PeerId &p, ConnectionSPtr c) override;

    void collectGarbage() override;

    void closeConnectionsToPeer(const peer::PeerId &p) override;

   private:
    std::shared_ptr<network::TransportManager> transport_manager_;

    std::unordered_map<peer::PeerId, std::vector<ConnectionSPtr>> connections_;

    std::shared_ptr<libp2p::event::Bus> bus_;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_CONNECTION_MANAGER_IMPL_HPP
