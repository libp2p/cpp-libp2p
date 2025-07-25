/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>
#include <vector>

#include "protocol_muxer.hpp"

namespace libp2p::basic {
  class Scheduler;
}

namespace libp2p::protocol_muxer::multiselect {

  class MultiselectInstance;

  /// Multiselect protocol implementation of ProtocolMuxer
  class Multiselect : public protocol_muxer::ProtocolMuxer {
   public:
    using Instance = std::shared_ptr<MultiselectInstance>;

    explicit Multiselect(std::shared_ptr<basic::Scheduler> scheduler);

    ~Multiselect() override = default;

    /// Implements ProtocolMuxer API
    void selectOneOf(std::span<const peer::ProtocolName> protocols,
                     std::shared_ptr<basic::ReadWriter> connection,
                     bool is_initiator,
                     bool negotiate_multiselect,
                     ProtocolHandlerFunc cb) override;

    /// Simple single stream negotiate procedure
    void simpleStreamNegotiate(
        const std::shared_ptr<connection::Stream> &stream,
        const peer::ProtocolName &protocol_id,
        std::function<void(
            outcome::result<std::shared_ptr<connection::Stream>>)> cb) override;

    /// Called from instance on close
    void instanceClosed(Instance instance,
                        const ProtocolHandlerFunc &cb,
                        outcome::result<peer::ProtocolName> result);

   private:
    /// Returns instance either from cache or creates a new one
    Instance getInstance();

    /// Scheduler for timeout management
    std::shared_ptr<basic::Scheduler> scheduler_;

    /// Active instances, keep them here to hold shared ptrs alive
    std::unordered_set<Instance> active_instances_;

    /// Idle instances which can be reused
    std::vector<Instance> cache_;
  };

}  // namespace libp2p::protocol_muxer::multiselect
