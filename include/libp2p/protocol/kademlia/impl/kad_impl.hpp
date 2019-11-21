/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_IMPL_HPP
#define LIBP2P_KAD_IMPL_HPP

#include <libp2p/network/network.hpp>
#include <libp2p/peer/peer_repository.hpp>
#include <libp2p/protocol/kademlia/kad.hpp>
#include <libp2p/protocol/kademlia/impl/kad_server_session_impl.hpp>
//#include <libp2p/protocol/kademlia/message_read_writer.hpp>
//#include <libp2p/protocol/kademlia/query_runner.hpp>
#include <libp2p/protocol/kademlia/routing_table.hpp>
#include <libp2p/peer/protocol.hpp>
#include <libp2p/protocol/base_protocol.hpp>
#include <libp2p/common/logger.hpp>

namespace libp2p::protocol::kademlia {

  class KadImpl : public Kad, public BaseProtocol {
   public:
    ~KadImpl() override = default;

    KadImpl(network::Network &network,
            std::shared_ptr<peer::PeerRepository> repo,
            std::shared_ptr<RoutingTable> table,
  //          std::shared_ptr<MessageReadWriter> mrw,
  //          std::shared_ptr<QueryRunner> runner,
            KademliaConfig config = {},
            KadServerSessionCreate server_session_create = KadServerSessionCreate());

    peer::PeerInfo findLocal(const peer::PeerId &id);

    void findPeer(peer::PeerId id, FindPeerResultFunc f) override;

    // NOLINTNEXTLINE(modernize-use-nodiscard)
    peer::Protocol getProtocolId() const override;

    // handle incoming stream
    void handle(StreamResult rstream) override;



   private:
    void onMessage(uint64_t from, outcome::result<Message> msg);

    void replyToFindNode(uint64_t to, Message&& msg);

    void closeSession(uint64_t id);

    network::Network &network_;
    std::shared_ptr<peer::PeerRepository> repo_;
    std::shared_ptr<RoutingTable> table_;
 //   std::shared_ptr<MessageReadWriter> mrw_;
 //   std::shared_ptr<QueryRunner> runner_;
    KademliaConfig config_;
    KadServerSessionCreate server_session_create_fn_;
    uint64_t sessions_counter_ = 0;
    struct Session {
      std::shared_ptr<connection::Stream> stream;
      std::shared_ptr<KadServerSession> session;
    };
    std::unordered_map<uint64_t, Session> sessions_;
    common::Logger log_ = common::createLogger("kad");
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_IMPL_HPP
