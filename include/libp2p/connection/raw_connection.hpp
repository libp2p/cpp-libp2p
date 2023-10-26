/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/connection/layer_connection.hpp>

namespace libp2p::connection {

  /**
   * @bried Represents raw network connection, before any upgrade
   */
  struct RawConnection : public virtual LayerConnection {
    ~RawConnection() override = default;
  };

}  // namespace libp2p::connection
