/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_MANAGER_MOCK_HPP
#define LIBP2P_CONNECTION_MANAGER_MOCK_HPP

#include <libp2p/network/connection_manager.hpp>

#include <vector>

#include <gmock/gmock.h>

namespace libp2p::network {

  struct ConnectionManagerMock : public ConnectionManager {
    ~ConnectionManagerMock() override = default;

    MOCK_CONST_METHOD0(getConnections, std::vector<ConnectionSPtr>());
    MOCK_CONST_METHOD1(getConnectionsToPeer,
                       std::vector<ConnectionSPtr>(const peer::PeerId &p));
    MOCK_CONST_METHOD1(getBestConnectionForPeer,
                       ConnectionSPtr(const peer::PeerId &p));

    MOCK_METHOD2(addConnectionToPeer,
                 void(const peer::PeerId &p, ConnectionSPtr c));

    MOCK_METHOD1(closeConnectionsToPeer, void(const peer::PeerId &p));

    MOCK_METHOD0(collectGarbage, void());

    MOCK_METHOD2(onConnectionClosed, void(
        const peer::PeerId &peer_id,
        const std::shared_ptr<connection::CapableConnection> &conn));
  };

}  // namespace libp2p::network

#endif  // LIBP2P_CONNECTION_MANAGER_MOCK_HPP
