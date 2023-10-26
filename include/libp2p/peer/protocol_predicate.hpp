/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>

#include <libp2p/peer/protocol.hpp>

namespace libp2p {
  /**
   * Protocol predicate.
   * Used by `Host::setProtocolHandler`, `Router::setProtocolHandler`.
   */
  using ProtocolPredicate = std::function<bool(const peer::ProtocolName &)>;
}  // namespace libp2p
