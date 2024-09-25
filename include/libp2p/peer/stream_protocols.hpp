/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include <libp2p/peer/protocol.hpp>

namespace libp2p {
  /**
   * Protocol list.
   * Used by `Host::newStream`, `Dialer::newStream`, `Host::setProtocolHandler`,
   * `Router::setProtocolHandler`.
   */
  struct StreamProtocols {
    std::vector<peer::ProtocolName> protocols;
    bool operator ==(const StreamProtocols &rhs) const {
      return protocols == rhs.protocols;
    }
  };

}  // namespace libp2p
