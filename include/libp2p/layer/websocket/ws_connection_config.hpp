/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>

namespace libp2p::layer {
  /**
   * Config of websocket layer connection
   */
  struct WsConnectionConfig {
    std::chrono::milliseconds ping_interval{60'000};
    std::chrono::milliseconds ping_timeout{10'000};
  };
}  // namespace libp2p::layer
