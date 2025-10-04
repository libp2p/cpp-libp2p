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

    /// Set replication interval
    /// @param interval - replication interval
    virtual void setReplicationInterval(std::chrono::seconds interval) = 0;

    /// Set republishing interval
    /// @param interval - republishing interval
    virtual void setRepublishingInterval(std::chrono::seconds interval) = 0;

    /// Enable/disable periodic replication
    /// @param enabled - whether to enable replication
    virtual void setReplicationEnabled(bool enabled) = 0;

    /// Enable/disable periodic republishing
    /// @param enabled - whether to enable republishing
    virtual void setRepublishingEnabled(bool enabled) = 0;
  };

}  // namespace libp2p::protocol::kademlia
