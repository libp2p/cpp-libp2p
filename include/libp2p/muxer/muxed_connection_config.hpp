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
   public:
    /// how much unconsumed data each stream can have stored locally
    size_t maximum_window_size = 3 * 16 * 1024 * 1024;

    /// how much streams can be supported by Yamux at one time
    size_t maximum_streams = 1000;
  };
}  // namespace libp2p::muxer

#endif  // LIBP2P_MUXED_CONNECTION_CONFIG_HPP
