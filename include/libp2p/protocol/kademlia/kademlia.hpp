/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/protocol/kademlia/routing.hpp>

namespace libp2p::protocol::kademlia {

  class Kademlia : public Routing {
   public:
    virtual ~Kademlia() = default;

    virtual void start() = 0;
  };

}  // namespace libp2p::protocol::kademlia
