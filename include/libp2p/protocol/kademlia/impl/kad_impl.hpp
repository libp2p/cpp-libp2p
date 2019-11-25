/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_IMPL_HPP
#define LIBP2P_KAD_IMPL_HPP

#include <libp2p/protocol/kademlia/kad.hpp>
#include <libp2p/protocol/kademlia/routing_table.hpp>
#include <libp2p/protocol/kademlia/impl/kad_protocol_session.hpp>
#include <libp2p/protocol/kademlia/impl/kad_response_handler.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/common/logger.hpp>

namespace libp2p::protocol::kademlia {

  class KadImpl : public Kad, public KadSessionHost, public std::enable_shared_from_this<KadImpl> {
   public:
    KadImpl(
      Host& host, std::shared_ptr<RoutingTable> table,
      KademliaConfig config = KademliaConfig{}
    );

    peer::Protocol getProtocolId() const override;

    void handle(StreamResult rstream) override;

    void start(bool start_server) override;

    void addPeer(peer::PeerInfo peer_info, bool permanent) override;

    bool findPeer(const peer::PeerId& peer, FindPeerQueryResultFunc f) override;

    bool findPeer(const peer::PeerId& peer, const std::unordered_set<peer::PeerInfo>& closer_peers, FindPeerQueryResultFunc f) override;

   private:
    enum SessionState {
      closed = KadProtocolSession::CLOSED_STATE,
      reading_from_peer, writing_to_peer };

    struct Session {
      KadProtocolSession::Ptr protocol_handler;

      // nullptr for server sessions
      KadResponseHandler::Ptr response_handler;
    };

    typedef bool (KadImpl::*RequestHandler)(Message& msg);

    Session* findSession(connection::Stream* from);

    void onMessage(connection::Stream* from, Message&& msg) override;

    void onCompleted(connection::Stream* from, outcome::result<void> res) override;

    const KademliaConfig& config() override;

    void newServerSession(std::shared_ptr<connection::Stream> stream);

    void closeSession(connection::Stream* s);

    void connect(const peer::PeerInfo& pi, const std::shared_ptr<KadResponseHandler>& handler,
                 const KadProtocolSession::Buffer& request);

    void onConnected(uint64_t id, const peer::PeerId& peerId, outcome::result<std::shared_ptr<connection::Stream>> stream_res,
                     KadProtocolSession::Buffer request);

    // request handlers
    bool onPutValue(Message& msg);
    bool onGetValue(Message& msg);
    bool onAddProvider(Message& msg);
    bool onGetProviders(Message& msg);
    bool onFindNode(Message& msg);
    bool onPing(Message& msg);

    KademliaConfig config_;
    Host& host_;
    std::shared_ptr<RoutingTable> table_;

    bool started_ = false;
    bool is_server_ = false;

    using Sessions = std::map<connection::Stream*, Session>;
    Sessions sessions_;

    using ConnectingSessions = std::map<uint64_t, KadResponseHandler::Ptr>;
    ConnectingSessions connecting_sessions_;
    uint64_t connecting_sessions_counter_ = 0;

    event::Handle new_channel_subscription_;

    static RequestHandler request_handlers_table[];

    common::Logger log_ = common::createLogger("kad");
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_IMPL_HPP
