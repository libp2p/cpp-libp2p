/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>

namespace libp2p::protocol::gossip {
  struct ScoreConfig {
    double zero = 0;
    /// The score threshold below which gossip propagation is suppressed;
    /// should be negative.
    double gossip_threshold = -10;
    /// The score threshold below which we shouldn't publish when using flood
    /// publishing (also applies to fanout peers); should be negative and <=
    /// `gossip_threshold`.
    double publish_threshold = -50;
    /// The score threshold below which message processing is suppressed
    /// altogether, implementing an effective graylist according to peer score;
    /// should be negative and
    /// <= `publish_threshold`.
    double graylist_threshold = -80;
    /// The median mesh score threshold before triggering opportunistic
    /// grafting; this should have a small positive value.
    double opportunistic_graft_threshold = 20;

    /// The decay interval for parameter counters.
    std::chrono::milliseconds decay_interval = std::chrono::seconds{1};

    bool valid() const {
      if (gossip_threshold > 0) {
        return false;
      }
      if (publish_threshold > 0 or publish_threshold > gossip_threshold) {
        return false;
      }
      if (graylist_threshold > 0 or graylist_threshold > publish_threshold) {
        return false;
      }
      if (opportunistic_graft_threshold < 0) {
        return false;
      }
      return true;
    }
  };
}  // namespace libp2p::protocol::gossip
