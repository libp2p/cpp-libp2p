/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace libp2p::protocol::gossip {
  enum class PeerKind : uint8_t {
    NotSupported,
    Floodsub,
    Gossipsub,
    Gossipsubv1_1,
    Gossipsubv1_2,
  };
}  // namespace libp2p::protocol::gossip
