/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_PUTVALUEEXECUTOR
#define LIBP2P_PROTOCOL_KADEMLIA_PUTVALUEEXECUTOR

#include <libp2p/protocol/kademlia/impl/response_handler.hpp>

#include <boost/multi_index/hashed_index_fwd.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container_fwd.hpp>
#include <memory>
#include <queue>
#include <unordered_set>

#include <libp2p/common/types.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/log/sublogger.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/impl/peer_id_with_distance.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/impl/session_host.hpp>
#include <libp2p/protocol/kademlia/peer_routing.hpp>

namespace libp2p::protocol::kademlia {

  class PutValueExecutor
      : public ResponseHandler,
        public std::enable_shared_from_this<PutValueExecutor> {
   public:
    PutValueExecutor(const Config &config, std::shared_ptr<Host> host,
                     std::shared_ptr<basic::Scheduler> scheduler,
                     std::shared_ptr<SessionHost> session_host, ContentId key,
                     ContentValue value, std::vector<PeerId> addressees);

    ~PutValueExecutor();

    outcome::result<void> start();

    /// @see ResponseHandler::responseTimeout
    Time responseTimeout() const override;

    /// @see ResponseHandler::match
    bool match(const Message &msg) const override;

    /// @see ResponseHandler::onResult
    void onResult(const std::shared_ptr<Session> &session,
                  outcome::result<Message> msg_res) override;

   private:
    /// Spawns new request
    void spawn();

    /// Handles result of connection
    void onConnected(StreamAndProtocolOrError stream_res);

    static std::atomic_size_t instance_number;

    // Primary
    const Config &config_;
    std::shared_ptr<Host> host_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<SessionHost> session_host_;

    // Secondary
    ContentId key_;
    ContentValue value_;

    // Auxiliary
    std::shared_ptr<std::vector<uint8_t>> serialized_request_;
    std::vector<PeerId> addressees_;
    size_t addressees_idx_ = 0;
    size_t requests_succeed_ = 0;
    size_t requests_in_progress_ = 0;
    bool started_ = false;
    bool done_ = false;

    log::SubLogger log_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_PUTVALUEEXECUTOR
