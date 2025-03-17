/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace libp2p::protocol::gossip {
  struct ScoreConfig {
    double zero = 0;
    double gossip_threshold = -10;
    double publish_threshold = -50;
  };
}  // namespace libp2p::protocol::gossip
