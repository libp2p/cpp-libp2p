/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_FINDPEEREXECUTOR
#define LIBP2P_PROTOCOL_KADEMLIA_FINDPEEREXECUTOR

#include <libp2p/protocol/kademlia/impl/response_handler.hpp>

#include <memory>
#include <queue>
#include <unordered_set>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/log/sublogger.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/impl/peer_id_with_distance.hpp>
#include <libp2p/protocol/kademlia/impl/peer_routing_table.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/impl/session_host.hpp>
#include <libp2p/protocol/kademlia/peer_routing.hpp>

namespace libp2p::protocol::kademlia {

  class FindPeerExecutor
      : public ResponseHandler,
        public std::enable_shared_from_this<FindPeerExecutor> {
   public:
    FindPeerExecutor(
        const Config &config, std::shared_ptr<Host> host,
        std::shared_ptr<basic::Scheduler> scheduler,
        std::shared_ptr<SessionHost> session_host,
        std::shared_ptr<PeerRouting> peer_routing,
        const std::shared_ptr<PeerRoutingTable> &peer_routing_table,
        PeerId peer_id, FoundPeerInfoHandler handler);

    ~FindPeerExecutor() override;

    outcome::result<void> start();

    void done(outcome::result<PeerInfo> result);

    /// @see ResponseHandler::responseTimeout
    Time responseTimeout() const override;

    /// @see ResponseHandler::match
    bool match(const Message &msg) const override;

    /// @see ResponseHandler::onResult
    void onResult(const std::shared_ptr<Session> &session,
                  outcome::result<Message> res) override;

   private:
    /// Spawns new request
    void spawn();

    /// Handles result of connection
    void onConnected(
        outcome::result<std::shared_ptr<connection::Stream>> stream_res);

    static std::atomic_size_t instance_number;

    // Primary
    const Config &config_;
    std::shared_ptr<Host> host_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<SessionHost> session_host_;
    std::shared_ptr<PeerRouting> peer_routing_;

    // Secondary
    const PeerId sought_peer_id_;
    const NodeId target_;
    std::unordered_set<PeerId> nearest_peer_ids_;
    FoundPeerInfoHandler handler_;

    // Auxiliary
    std::shared_ptr<std::vector<uint8_t>> serialized_request_;
    std::priority_queue<PeerIdWithDistance> queue_;
    size_t requests_in_progress_ = 0;
    bool started_ = false;
    std::atomic_bool done_ = false;

    log::SubLogger log_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_FINDPEEREXECUTOR
