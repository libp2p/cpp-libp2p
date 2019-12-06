/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_SERVER_HPP
#define LIBP2P_KAD_SERVER_HPP

#include <libp2p/protocol/kademlia/impl/kad_impl.hpp>
#include <libp2p/protocol/kademlia/impl/kad_message.hpp>

namespace libp2p::protocol::kademlia {

  class KadServer : public protocol::BaseProtocol,
                    public KadSessionHost,
                    public std::enable_shared_from_this<KadServer> {
   public:
    KadServer(Host &host, KadImpl& kad);

    ~KadServer();

   private:
    // BaseProtocol overrides
    peer::Protocol getProtocolId() const override {
      return protocol_;
    }
    void handle(StreamResult rstream) override;

    // KadBackend overrides
    const KademliaConfig &config() override {
      return kad_.config();
    }
    Scheduler &scheduler() override {
      return kad_.scheduler();
    }

    PeerIdVec getNearestPeers(const NodeId& id) override {
      return kad_.getNearestPeers(id);
    }

    enum SessionState {
      closed = KadProtocolSession::CLOSED_STATE,
      reading_from_peer,
      writing_to_peer
    };

    KadProtocolSession::Ptr findSession(connection::Stream *from);

    void onMessage(connection::Stream *from, Message &&msg) override;

    void onCompleted(connection::Stream *from,
                     outcome::result<void> res) override;

    void closeSession(connection::Stream *s);

    // request handlers
    bool onPutValue(Message &msg);
    bool onGetValue(Message &msg);
    bool onAddProvider(Message &msg);
    bool onGetProviders(Message &msg);
    bool onFindNode(Message &msg);
    bool onPing(Message &msg);

    Host &host_;
    KadImpl& kad_;
    const peer::Protocol protocol_;

    using Sessions = std::map<connection::Stream *, KadProtocolSession::Ptr>;
    Sessions sessions_;

    SubLogger log_;

    using RequestHandler = bool (KadServer::*)(Message &);
    static std::array<RequestHandler, Message::kTableSize> request_handlers_table;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_SERVER_HPP
