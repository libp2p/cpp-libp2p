/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
  };
}  // namespace libp2p::protocol::gossip
