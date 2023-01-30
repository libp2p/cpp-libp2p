/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PEER_PROTOCOL_PREDICATE_HPP
#define LIBP2P_PEER_PROTOCOL_PREDICATE_HPP

#include <functional>

#include <libp2p/peer/protocol.hpp>

namespace libp2p {
  /**
   * Protocol predicate.
   * Used by `Host::setProtocolHandler`, `Router::setProtocolHandler`.
   */
  using ProtocolPredicate = std::function<bool(const peer::ProtocolName &)>;
}  // namespace libp2p

#endif  // LIBP2P_PEER_PROTOCOL_PREDICATE_HPP
