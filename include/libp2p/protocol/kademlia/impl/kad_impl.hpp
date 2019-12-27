/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_IMPL_HPP
#define LIBP2P_KAD_IMPL_HPP

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/kademlia/impl/content_providers_store.hpp>
#include <libp2p/protocol/kademlia/impl/kad_protocol_session.hpp>
#include <libp2p/protocol/kademlia/impl/kad_response_handler.hpp>
#include <libp2p/protocol/kademlia/impl/kad_session_host.hpp>
#include <libp2p/protocol/kademlia/impl/local_value_store.hpp>
#include <libp2p/protocol/kademlia/kad.hpp>
#include <libp2p/protocol/kademlia/routing_table.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/common/helpers.hpp>

namespace libp2p::protocol::kademlia {

  class KadServer;

  class KadImpl : public Kad,
                  public KadSessionHost,
                  public std::enable_shared_from_this<KadImpl> {
   public:
    KadImpl(std::shared_ptr<Host> host, std::shared_ptr<Scheduler> scheduler,
            std::shared_ptr<RoutingTable> table,
            std::unique_ptr<ValueStoreBackend> storage, KademliaConfig config);

    ~KadImpl() override;

    void start(bool start_server) override;

    void addPeer(peer::PeerInfo peer_info, bool permanent) override;

    bool findPeer(const peer::PeerId &peer, FindPeerQueryResultFunc f) override;

    bool findPeer(const peer::PeerId &peer,
                  const std::unordered_set<peer::PeerInfo> &closer_peers,
                  FindPeerQueryResultFunc f) override;

    void putValue(const ContentAddress &key, Value value,
                  PutValueResultFunc f) override;

    void getValue(const ContentAddress &key, GetValueResultFunc f) override;

    void broadcastThisProvider(const ContentAddress &key) override;

    const KademliaConfig &config() override {
      return config_;
    }

    Scheduler &scheduler() override {
      return *scheduler_;
    }

    PeerIdVec getNearestPeers(const NodeId &id) override;

    LocalValueStore &getLocalValueStore() {
      return *local_store_;
    }

    ContentProvidersStore &getContentProvidersStore() {
      return providers_store_;
    }

   private:
    enum SessionState {
      closed = KadProtocolSession::CLOSED_STATE,
      reading_from_peer,
      writing_to_peer
    };

    struct Session {
      KadProtocolSession::Ptr protocol_handler;

      // nullptr for server sessions
      KadResponseHandler::Ptr response_handler;
    };

    Session *findSession(connection::Stream *from);

    void onMessage(connection::Stream *from, Message &&msg) override;

    void onCompleted(connection::Stream *from,
                     outcome::result<void> res) override;

    void closeSession(connection::Stream *s);

    void connect(const peer::PeerInfo &pi,
                 const std::shared_ptr<KadResponseHandler> &handler,
                 const KadProtocolSession::Buffer &request);

    void onConnected(
        uint64_t id, const peer::PeerId &peerId,
        outcome::result<std::shared_ptr<connection::Stream>> stream_res,
        KadProtocolSession::Buffer request);

    KademliaConfig config_;
    const peer::Protocol protocol_;
    std::shared_ptr<Host> host_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<RoutingTable> table_;
    std::unique_ptr<LocalValueStore> local_store_;
    ContentProvidersStore providers_store_;
    std::unique_ptr<KadServer> server_;

    bool started_ = false;

    using Sessions = std::map<connection::Stream *, Session>;
    Sessions sessions_;

    using ConnectingSessions = std::map<uint64_t, KadResponseHandler::Ptr>;
    ConnectingSessions connecting_sessions_;
    uint64_t connecting_sessions_counter_ = 0;

    event::Handle new_channel_subscription_;
    SubLogger log_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_IMPL_HPP
