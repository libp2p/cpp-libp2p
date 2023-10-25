/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/protocol.hpp>

namespace libp2p::basic {
  /**
   * Base class of adaptor - entity, which encapsulates logic of upgrading the
   * connection
   */
  struct Adaptor {
    virtual ~Adaptor() = default;

    /**
     * Get a string identifier, associated with this adaptor
     * @return protocol id of the adaptor
     * @example '/yamux/1.0.0' or '/plaintext/1.5.0'
     */
    virtual peer::ProtocolName getProtocolId() const = 0;
  };
}  // namespace libp2p::basic
