/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PEER_STREAM_PROTOCOLS_HPP
#define LIBP2P_PEER_STREAM_PROTOCOLS_HPP

#include <vector>

#include <libp2p/peer/protocol.hpp>

namespace libp2p {
  /**
   * Protocol list.
   * Used by `Host::newStream`, `Dialer::newStream`, `Host::setProtocolHandler`,
   * `Router::setProtocolHandler`.
   */
  using StreamProtocols = std::vector<peer::ProtocolName>;
}  // namespace libp2p

#endif  // LIBP2P_PEER_STREAM_PROTOCOLS_HPP
