/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MUXED_CONNECTION_CONFIG_HPP
#define LIBP2P_MUXED_CONNECTION_CONFIG_HPP

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
  };
}  // namespace libp2p::muxer

#endif  // LIBP2P_MUXED_CONNECTION_CONFIG_HPP
