/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_IMPL_HPP
#define LIBP2P_KAD_IMPL_HPP

#include <libp2p/network/network.hpp>
#include <libp2p/peer/peer_repository.hpp>
#include <libp2p/protocol/kademlia/kad.hpp>
#include <libp2p/protocol/kademlia/message_read_writer.hpp>
#include <libp2p/protocol/kademlia/query_runner.hpp>
#include <libp2p/protocol/kademlia/routing_table.hpp>

namespace libp2p::protocol::kademlia {

  class KadImpl : public Kad {
   public:
    ~KadImpl() override = default;

    KadImpl(network::Network &network,
            std::shared_ptr<peer::PeerRepository> repo,
            std::shared_ptr<RoutingTable> table,
            std::shared_ptr<MessageReadWriter> mrw,
            std::shared_ptr<QueryRunner> runner, KademliaConfig config = {});

    peer::PeerInfo findLocal(const peer::PeerId &id);

    void findPeer(peer::PeerId id, FindPeerResultFunc f) override;

   private:
    network::Network &network_;
    std::shared_ptr<peer::PeerRepository> repo_;
    std::shared_ptr<RoutingTable> table_;
    std::shared_ptr<MessageReadWriter> mrw_;
    std::shared_ptr<QueryRunner> runner_;
    KademliaConfig config_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_IMPL_HPP
