/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_RAW_CONNECTION_HPP
#define LIBP2P_RAW_CONNECTION_HPP

#include <libp2p/connection/layer_connection.hpp>

namespace libp2p::connection {

  /**
   * @bried Represents raw network connection, before any upgrade
   */
  struct RawConnection : public virtual LayerConnection {
    ~RawConnection() override = default;
  };

}  // namespace libp2p::connection

#endif  // LIBP2P_RAW_CONNECTION_HPP
