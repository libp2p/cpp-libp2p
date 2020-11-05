/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_FINDPROVIDERSEXECUTOR
#define LIBP2P_PROTOCOL_KADEMLIA_FINDPROVIDERSEXECUTOR

#include <memory>
#include <queue>
#include <unordered_set>

#include <libp2p/common/types.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/common/sublogger.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/impl/peer_info_with_distance.hpp>
#include <libp2p/protocol/kademlia/impl/response_handler.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/impl/session_host.hpp>
#include <libp2p/protocol/kademlia/peer_routing.hpp>

namespace libp2p::protocol::kademlia {

  class GetProvidersExecutor
      : public ResponseHandler,
        public std::enable_shared_from_this<GetProvidersExecutor> {
   public:
    GetProvidersExecutor(const Config &config, std::shared_ptr<Host> host,
                         std::shared_ptr<SessionHost> session_host,
                         std::shared_ptr<PeerRouting> peer_routing,
                         ContentId sought_key,
                         std::unordered_set<PeerInfo> nearest_peer_infos,
                         FoundProvidersHandler handler);

    ~GetProvidersExecutor() override;

    outcome::result<void> start();

    /// @see ResponseHandler::responseTimeout
    scheduler::Ticks responseTimeout() const override {
      return scheduler::toTicks(10s);  // TODO(xDimon): Need to read from config
    }

    /// @see ResponseHandler::match
    bool match(const Message &msg) const override;

    /// @see ResponseHandler::onResult
    void onResult(const std::shared_ptr<Session> &session,
                  outcome::result<Message> msg_res) override;

   private:
    /// Spawns new request
    void spawn();

    /// Handles result of connection
    void onConnected(
        outcome::result<std::shared_ptr<connection::Stream>> stream_res);

    static std::atomic_size_t instance_number;

    const Config &config_;
    std::shared_ptr<Host> host_;
    std::shared_ptr<SessionHost> session_host_;
    std::shared_ptr<PeerRouting> peer_routing_;
    const Key sought_content_id_;
    size_t required_providers_amount_ = 1;
    std::unordered_set<PeerInfo, std::hash<PeerInfo>, PeerInfo::EqualByPeerId>
        nearest_peer_infos_;
    FoundProvidersHandler handler_;

    std::shared_ptr<std::vector<uint8_t>> serialized_request_;
    std::priority_queue<PeerInfoWithDistance> queue_;
    size_t requests_in_progress_ = 0;
    bool started_ = false;
    bool done_ = false;

    std::unordered_set<PeerId> providers_;

    SubLogger log_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_FINDPROVIDERSEXECUTOR
