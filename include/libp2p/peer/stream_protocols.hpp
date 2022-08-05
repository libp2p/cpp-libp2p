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
   * `Router::setProtocolHandler`. Overloads argument constructor instead of
   * `newStream` and `setProtocolHandler`.
   */
  struct StreamProtocols : std::vector<peer::Protocol> {
    StreamProtocols(const char *protocol) : vector{protocol} {}
    StreamProtocols(peer::Protocol &&protocol) : vector{std::move(protocol)} {}
    StreamProtocols(const peer::Protocol &protocol) : vector{protocol} {}
    StreamProtocols(std::vector<peer::Protocol> &&protocols)
        : vector{std::move(protocols)} {}
  };
}  // namespace libp2p

#endif  // LIBP2P_PEER_STREAM_PROTOCOLS_HPP
