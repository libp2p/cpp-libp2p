/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>

#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/connection/stream_and_protocol.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/protocol.hpp>
#include <libp2p/peer/protocol_predicate.hpp>
#include <libp2p/peer/stream_protocols.hpp>

namespace libp2p::network {

  /**
   * @brief Class, which is capable of opening new connections and streams using
   * registered transports.
   */
  struct Dialer {
    virtual ~Dialer() = default;

    using DialResult =
        outcome::result<std::shared_ptr<connection::CapableConnection>>;
    using DialResultFunc = std::function<void(DialResult)>;

    /**
     * Establishes a connection or returns existing one to a given peer with a
     * specific timeout
     */
    virtual void dial(const PeerInfo &p, DialResultFunc cb) = 0;

    /**
     * NewStream returns a new stream to given peer p.
     * If there is no connection to p, returns error.
     */
    virtual void newStream(const PeerInfo &peer_id,
                           StreamProtocols protocols,
                           StreamAndProtocolOrErrorCb cb) = 0;

    void newStream(const PeerId &peer_id,
                   StreamProtocols protocols,
                   StreamAndProtocolOrErrorCb cb) {
      newStream(PeerInfo{.id = peer_id}, std::move(protocols), std::move(cb));
    }
  };

}  // namespace libp2p::network
