/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL
#define LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL

#include <libp2p/protocol/kademlia/impl/executors_factory.hpp>
#include <libp2p/protocol/kademlia/impl/session_host.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>

#include <unordered_map>

#include <libp2p/crypto/random_generator.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/log/sublogger.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/impl/content_routing_table.hpp>
#include <libp2p/protocol/kademlia/impl/peer_routing_table.hpp>
#include <libp2p/protocol/kademlia/impl/storage.hpp>
#include <libp2p/protocol/kademlia/validator.hpp>

namespace libp2p::protocol::kademlia {

  class PutValueExecutor;
  class FindPeerExecutor;
  class FindProvidersExecutor;

  class KademliaImpl final : public Kademlia,
                             public SessionHost,
                             public ExecutorsFactory,
                             public std::enable_shared_from_this<KademliaImpl> {
   public:
    KademliaImpl(
        const Config &config, std::shared_ptr<Host> host,
        std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentRoutingTable> content_routing_table,
        std::shared_ptr<PeerRoutingTable> peer_routing_table,
        std::shared_ptr<Validator> validator,
        std::shared_ptr<basic::Scheduler> scheduler,
        std::shared_ptr<event::Bus> bus,
        std::shared_ptr<crypto::random::RandomGenerator> random_generator);

    ~KademliaImpl() override = default;

    /// @see Kademlia::start
    void start() override;

    /// @see Routing::bootstrap
    outcome::result<void> bootstrap() override;

    /// @see ValueStore::putValue
    outcome::result<void> putValue(Key key, Value value) override;

    /// @see ValueStore::getValue
    outcome::result<void> getValue(const Key &key,
                                   FoundValueHandler handler) override;

    /// @see ContentRouting::provide
    outcome::result<void> provide(const Key &key, bool need_notify) override;

    /// @see ContentRouting::findProviders
    outcome::result<void> findProviders(const Key &key, size_t limit,
                                        FoundProvidersHandler handler) override;

    /// @see PeerRouting::addPeer
    void addPeer(const PeerInfo &peer_info, bool permanent,
                 bool is_connected = false) override;

    /// @see PeerRouting::findPeer
    outcome::result<void> findPeer(const peer::PeerId &peer_id,
                                   FoundPeerInfoHandler handler) override;

    /// @see MessageObserver::onMessage
    void onMessage(const std::shared_ptr<Session> &stream,
                   Message &&msg) override;

    /// @see SessionHost::openSession
    std::shared_ptr<Session> openSession(
        std::shared_ptr<connection::Stream> stream) override;

    /// @see SessionHost::closeSession
    void closeSession(std::shared_ptr<connection::Stream> stream) override;

   private:
    void onPutValue(const std::shared_ptr<Session> &session, Message &&msg);
    void onGetValue(const std::shared_ptr<Session> &session, Message &&msg);
    void onAddProvider(const std::shared_ptr<Session> &session, Message &&msg);
    void onGetProviders(const std::shared_ptr<Session> &session, Message &&msg);
    void onFindNode(const std::shared_ptr<Session> &session, Message &&msg);
    void onPing(const std::shared_ptr<Session> &session, Message &&msg);

    void handleProtocol(StreamAndProtocol stream);

    std::shared_ptr<PutValueExecutor> createPutValueExecutor(
        ContentId key, ContentValue value,
        std::vector<PeerId> addressees) override;

    std::shared_ptr<GetValueExecutor> createGetValueExecutor(
        ContentId key, FoundValueHandler handler) override;

    std::shared_ptr<AddProviderExecutor> createAddProviderExecutor(
        ContentId content_id) override;

    std::shared_ptr<FindProvidersExecutor> createGetProvidersExecutor(
        ContentId content_id, FoundProvidersHandler handler) override;

    std::shared_ptr<FindPeerExecutor> createFindPeerExecutor(
        PeerId peer_id, FoundPeerInfoHandler handler) override;

    outcome::result<void> findRandomPeer();
    void randomWalk();

    // --- Primary (Injected) ---

    const Config &config_;
    std::shared_ptr<Host> host_;
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<ContentRoutingTable> content_routing_table_;
    std::shared_ptr<PeerRoutingTable> peer_routing_table_;
    std::shared_ptr<Validator> validator_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<event::Bus> bus_;
    std::shared_ptr<crypto::random::RandomGenerator> random_generator_;

    // --- Secondary ---

    const ProtocolName &protocol_;
    const PeerId self_id_;

    // --- Auxiliary ---

    // Flag if started early
    bool started_ = false;

    // Subscribtion to new connections
    event::Handle new_connection_subscription_;

    struct StreamPtrComparator {
      bool operator()(const std::shared_ptr<connection::Stream> &lhs,
                      const std::shared_ptr<connection::Stream> &rhs) const {
        return lhs.get() == rhs.get();
      }
    };

    struct StreamPtrHaher {
      size_t operator()(const std::shared_ptr<connection::Stream> &s) const {
        auto r = std::hash<decltype(s.get())>()(s.get());
        return r;
      }
    };

    std::unordered_map<const std::shared_ptr<connection::Stream>,
                       std::shared_ptr<Session>, StreamPtrHaher,
                       StreamPtrComparator>
        sessions_;

    // Random walk's auxiliary data
    struct {
      size_t iteration = 0;
      basic::Scheduler::Handle handle{};
    } random_walking_;

    log::SubLogger log_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL
