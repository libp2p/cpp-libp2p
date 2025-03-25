/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

namespace libp2p::protocol::gossip {
  class Score {
   public:
    bool below(const PeerId &peer_id, double threshold) {
      return false;
    }
    void addPenalty(const PeerId &peer_id, size_t count) {}
  };
}  // namespace libp2p::protocol::gossip
