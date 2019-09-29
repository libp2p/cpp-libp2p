/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PING_CONFIG_HPP
#define LIBP2P_PING_CONFIG_HPP

#include <cstdint>

namespace libp2p::protocol {
  struct PingConfig {
    /// now much Ping is going to wait before declaring other node dead (in
    /// ms)
    uint32_t timeout = 30000;

    /// size of the Ping message to be sent or received (in bytes)
    uint32_t message_size = 32;
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_PING_CONFIG_HPP
