/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL
#define LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL

#include <libp2p/crypto/random_generator.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/common/sublogger.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>
#include <libp2p/protocol/kademlia/routing_table.hpp>
#include <libp2p/protocol/kademlia/value_store.hpp>

namespace libp2p::protocol::kademlia {

  class KademliaImpl final : public Kademlia,
                             public std::enable_shared_from_this<KademliaImpl> {
   public:
    KademliaImpl(
        const Config &config, std::shared_ptr<Host> host,
        std::shared_ptr<ValueStore> storage,
        std::shared_ptr<RoutingTable> table,
        std::shared_ptr<MessageObserver> message_observer,
        std::shared_ptr<Scheduler> scheduler,
        std::shared_ptr<crypto::random::RandomGenerator> random_generator);

    ~KademliaImpl() override = default;

    void start() override;

    /// @see Routing::addPeer
    void addPeer(const PeerInfo &peer_info, bool permanent) override;

    /// @see Routing::bootstrap
    outcome::result<void> bootstrap() override;

    /// @see ValueStore::putValue
    outcome::result<void> putValue(Key key, Value value) override;

    /// @see ValueStore::getValue
    outcome::result<ValueAndTime> getValue(const Key &key) const override;

    /// @see ContentRouting::provide
    outcome::result<void> provide(const Key &key, bool need_notify) override;

    /// @see ContentRouting::findProviders
    outcome::result<void> findProviders(const Key &key, size_t limit,
                                        FoundProvidersHandler handler) override;

    /// @see PeerRouting::findPeer
    outcome::result<void> findPeer(const peer::PeerId &peer_id,
                                   FoundPeerInfoHandler handler) override;

    /// @see MessageObserver::onMessage
    void onMessage(connection::Stream *from, struct Message &&msg) override;

    /// @see MessageObserver::onMessage
    void onCompleted(connection::Stream *from,
                     outcome::result<void> res) override;

    /// @see MessageObserver::onPutValue
    bool onPutValue(Message &msg) override;

    /// @see MessageObserver::onGetValue
    bool onGetValue(Message &msg) override;

    /// @see MessageObserver::onAddProvider
    bool onAddProvider(Message &msg) override;

    /// @see MessageObserver::onGetProviders
    bool onGetProviders(Message &msg) override;

    /// @see MessageObserver::onFindNode
    bool onFindNode(Message &msg) override;

    /// @see MessageObserver::onPing
    bool onPing(Message &msg) override;

   private:
    std::vector<PeerId> getNearestPeers(const NodeId &id);

    //    struct FindPeerQueryResult {
    //      std::unordered_set<PeerInfo> closer_peers{};
    //      boost::optional<PeerInfo> peer{};
    //      bool success = false;
    //    };
    //
    //    using FindPeerQueryResultFunc =
    //        std::function<void(const peer::PeerId &peer,
    //        FindPeerQueryResult)>;
    //
    void findRandomPeer();
    void randomWalk();

    // --- Primary (Injected) ---

    const Config &config_;
    std::shared_ptr<Host> host_;
    std::shared_ptr<ValueStore> storage_;
    std::shared_ptr<RoutingTable> table_;
    std::shared_ptr<MessageObserver> message_observer_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<crypto::random::RandomGenerator> random_generator_;

    // --- Secondary ---

    const Protocol protocol_;
    const PeerId self_id_;

    //	ContentProvidersStore providers_store_;
    //	std::shared_ptr<KadServer> server_;
    //
    //	bool started_ = false;
    //
    //	using Sessions = std::map<connection::Stream *, Session>;
    //	Sessions sessions_;
    //
    //	using ConnectingSessions = std::map<uint64_t, KadResponseHandler::Ptr>;
    //	ConnectingSessions connecting_sessions_;
    //	uint64_t connecting_sessions_counter_ = 0;

    // --- Auxiliary ---

    // Flagg if started early
    bool started_ = false;

    // Subscribtion to new connections
    event::Handle new_channel_subscription_;

    // Random walk's auxiliary data
    struct {
      size_t iteration = 0;
      scheduler::Handle handle{};
    } random_walking_;

    SubLogger log_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL
