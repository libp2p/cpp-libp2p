/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <cstdint>

namespace libp2p::protocol {
  struct PingConfig {
    /// now much Ping is going to wait before declaring other node dead (in
    /// ms)
    std::chrono::milliseconds timeout = std::chrono::seconds{30};

    std::chrono::milliseconds interval = std::chrono::seconds{30};

    /// size of the Ping message to be sent or received (in bytes)
    uint32_t message_size = 32;
  };
}  // namespace libp2p::protocol
