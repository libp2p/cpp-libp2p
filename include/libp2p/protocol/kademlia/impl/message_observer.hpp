/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_MESSAGEOBSERVER
#define LIBP2P_PROTOCOL_KADEMLIA_MESSAGEOBSERVER

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/protocol/kademlia/message.hpp>

namespace libp2p::protocol::kademlia {

  class Session;

  class MessageObserver {
   public:
    virtual ~MessageObserver() = default;

    /// Handles inbound message
    virtual void onMessage(const std::shared_ptr<Session> &stream,
                           Message &&msg) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_MESSAGEOBSERVER
