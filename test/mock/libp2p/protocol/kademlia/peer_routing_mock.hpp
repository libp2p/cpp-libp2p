/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_ROUTINGTABLEMOCK
#define LIBP2P_PROTOCOL_KADEMLIA_ROUTINGTABLEMOCK

#include <libp2p/protocol/kademlia/peer_routing.hpp>

#include <gmock/gmock.h>

namespace libp2p::protocol::kademlia {

  struct RoutingTableMock : public PeerRouting {
    ~RoutingTableMock() override = default;

    MOCK_METHOD1(update, outcome::result<void>(const peer::PeerId &));
    MOCK_METHOD1(remove, void(const peer::PeerId &id));
    MOCK_CONST_METHOD0(getAllPeers, PeerIdVec());
    MOCK_METHOD2(getNearestPeers, PeerIdVec(const NodeId &id, size_t count));
    MOCK_CONST_METHOD0(size, size_t());
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_ROUTINGTABLEMOCK
