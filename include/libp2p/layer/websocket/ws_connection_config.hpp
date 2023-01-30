/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LAYER_WSCONNECTIONCONFIG
#define LIBP2P_LAYER_WSCONNECTIONCONFIG

#include <libp2p/layer/layer_connection_config.hpp>

namespace libp2p::layer {
  /**
   * Config of websocket layer connection
   */
  struct WsConnectionConfig : public LayerConnectionConfig {
    std::chrono::milliseconds ping_interval{60'000};
  };
}  // namespace libp2p::layer

#endif  // LIBP2P_LAYER_WSCONNECTIONCONFIG
