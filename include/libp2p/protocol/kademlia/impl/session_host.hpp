/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/protocol/kademlia/impl/message_observer.hpp>

namespace libp2p::protocol::kademlia {

  class Session;

  class SessionHost : public MessageObserver {
   public:
    virtual ~SessionHost() = default;

    /// Opens new session for stream
    virtual std::shared_ptr<Session> openSession(
        std::shared_ptr<connection::Stream> stream) = 0;

    /// Closes session by stream
    virtual void closeSession(std::shared_ptr<connection::Stream> stream) = 0;
  };

}  // namespace libp2p::protocol::kademlia
