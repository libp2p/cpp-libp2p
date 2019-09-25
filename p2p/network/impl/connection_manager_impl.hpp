/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_MANAGER_IMPL_HPP
#define KAGOME_CONNECTION_MANAGER_IMPL_HPP

#include "network/connection_manager.hpp"
#include "network/transport_manager.hpp"
#include "peer/peer_id.hpp"

namespace libp2p::network {

  class ConnectionManagerImpl : public ConnectionManager {
   public:
    explicit ConnectionManagerImpl(std::shared_ptr<network::TransportManager> tmgr);

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
  };

}  // namespace libp2p::network

#endif  // KAGOME_CONNECTION_MANAGER_IMPL_HPP
