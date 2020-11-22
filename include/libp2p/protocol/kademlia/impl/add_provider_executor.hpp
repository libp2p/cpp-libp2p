/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_ADDPROVIDEREXECUTOR
#define LIBP2P_PROTOCOL_KADEMLIA_ADDPROVIDEREXECUTOR

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

  class AddProviderExecutor
      : public std::enable_shared_from_this<AddProviderExecutor> {
   public:
    AddProviderExecutor(const Config &config, std::shared_ptr<Host> host,
                        std::shared_ptr<Scheduler> scheduler,

                        std::shared_ptr<SessionHost> session_host,
                        ContentId key, std::vector<PeerInfo> addressees);

    ~AddProviderExecutor();

    outcome::result<void> start();

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
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<SessionHost> session_host_;
    ContentId key_;
    std::vector<PeerInfo> addressees_;

    size_t addressees_idx_ = 0;

    // Secondary

    std::shared_ptr<std::vector<uint8_t>> serialized_request_;
    size_t requests_in_progress_ = 0;

    bool started_ = false;
    bool done_ = false;

    SubLogger log_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_ADDPROVIDEREXECUTOR
