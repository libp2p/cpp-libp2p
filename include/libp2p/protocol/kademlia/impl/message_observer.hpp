/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
