/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MUXED_CONNECTION_CONFIG_HPP
#define KAGOME_MUXED_CONNECTION_CONFIG_HPP

#include <cstddef>
#include <cstdint>

namespace libp2p::muxer {
  /**
   * Config of muxed connection
   */
  struct MuxedConnectionConfig {
   public:
    static constexpr uint32_t kDefaultMaximumWindowSize = 1024u * 1024u;
    static constexpr uint32_t kDefaultMaximumStreams = 1000u;

    /**
     * @brief construcsts configuration instance
     * @param max_window_size how much unconsumed data each stream can
     * have stored locally
     * @param max_streams how much streams can be supported by Yamux at
     * one time
     */
    inline MuxedConnectionConfig(uint32_t max_window_size, uint32_t max_streams)
        : maximum_window_size{max_window_size}, maximum_streams{max_streams} {}

    inline MuxedConnectionConfig()
        : maximum_window_size{kDefaultMaximumWindowSize},
          maximum_streams{kDefaultMaximumStreams} {}

    /// how much unconsumed data each stream can have stored locally
    size_t maximum_window_size;

    /// how much streams can be supported by Yamux at one time
    size_t maximum_streams;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_MUXED_CONNECTION_CONFIG_HPP
