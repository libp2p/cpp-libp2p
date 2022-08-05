/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_NETWORK_DIALER_HPP
#define LIBP2P_NETWORK_DIALER_HPP

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
    virtual void dial(const peer::PeerInfo &p, DialResultFunc cb,
                      std::chrono::milliseconds timeout) = 0;

    /**
     * Establishes a connection or returns existing one to a given peer
     */
    inline void dial(const peer::PeerInfo &p, DialResultFunc cb) {
      dial(p, std::move(cb), std::chrono::milliseconds::zero());
    }

    /**
     * NewStream returns a new stream to given peer p with a specific timeout.
     * If there is no connection to p, attempts to create one.
     */
    virtual void newStream(const peer::PeerInfo &peer_info,
                           StreamProtocols protocols,
                           StreamAndProtocolOrErrorCb cb,
                           std::chrono::milliseconds timeout = {}) = 0;

    /**
     * NewStream returns a new stream to given peer p.
     * If there is no connection to p, returns error.
     */
    virtual void newStream(const peer::PeerId &peer_id,
                           StreamProtocols protocols,
                           StreamAndProtocolOrErrorCb cb) = 0;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_NETWORK_DIALER_HPP
