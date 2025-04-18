/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace libp2p::muxer {
  /**
   * Config of muxed connection
   */
  struct MuxedConnectionConfig {
    /// how much unconsumed data each stream can have stored locally
    static constexpr size_t kDefaultMaxWindowSize = 64 * 1024 * 1024;
    size_t maximum_window_size = kDefaultMaxWindowSize;

    /// how much streams can be supported by Yamux at one time
    static constexpr size_t kDefaultMaxStreamsNumber = 1000;
    size_t maximum_streams = kDefaultMaxStreamsNumber;

    /// Ping interval for muxers with internal ping feature
    static constexpr std::chrono::milliseconds kDefaultPingInterval =
        std::chrono::milliseconds(5000);
    std::chrono::milliseconds ping_interval = kDefaultPingInterval;

    /// No streams interval after which connection closes itself
    static constexpr std::chrono::milliseconds kDefaultNoStreamsInterval =
        std::chrono::milliseconds(120000);
    std::chrono::milliseconds no_streams_interval = kDefaultNoStreamsInterval;

    /// Dial timeout for outgoing connection
    static constexpr std::chrono::seconds kDefaultDialTimeout{10};
    std::chrono::milliseconds dial_timeout = kDefaultDialTimeout;
  };
}  // namespace libp2p::muxer
