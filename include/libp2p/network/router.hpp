/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_ROUTER_HPP
#define LIBP2P_ROUTER_HPP

#include <functional>
#include <vector>

#include <libp2p/connection/stream.hpp>
#include <libp2p/connection/stream_and_protocol.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/peer/protocol.hpp>
#include <libp2p/peer/protocol_predicate.hpp>
#include <libp2p/peer/stream_protocols.hpp>

namespace libp2p::event::network {

  using ProtocolsAddedChannel =
      channel_decl<struct ProtocolsAdded, std::vector<peer::ProtocolName>>;

  using ProtocolsRemovedChannel =
      channel_decl<struct ProtocolsRemoved, std::vector<peer::ProtocolName>>;

}  // namespace libp2p::event::network

namespace libp2p::network {

  /**
   * Manager for application-level protocols; when a new stream arrives for a
   * specific protocol, the corresponding handler is called
   * @note analog of Go's switch:
   * https://github.com/libp2p/go-libp2p-core/blob/consolidate-skeleton/host/host.go#L37
   */
  struct Router {
    virtual ~Router() = default;

    /**
     * Set handler for the <protocol, predicate> pair. First, searches all
     * handlers by prefix of the given protocol, then executes handler callback
     * for all matches when {@param predicate} returns true
     * @param protocols, for which pairs <protocol, handler> are to be retrieved
     * @param cb to be executed over the protocols, for which the predicate
     * was evaluated to true
     * @param predicate to be executed over the found protocols
     * @see Host::setProtocolHandler(...) for examples
     */
    virtual void setProtocolHandler(StreamProtocols protocols,
                                    StreamAndProtocolCb cb,
                                    ProtocolPredicate predicate = {}) = 0;

    /**
     * Get a list of handled protocols
     * @return supported protocols; may also include protocol prefixes; if any
     * set
     */
    virtual std::vector<peer::ProtocolName> getSupportedProtocols() const = 0;

    /**
     * Remove handlers, associated with the given protocol prefix
     * @param protocol prefix, for which the handlers are to be removed
     */
    virtual void removeProtocolHandlers(const peer::ProtocolName &protocol) = 0;

    /**
     * Remove all handlers
     */
    virtual void removeAll() = 0;

    /**
     * Execute stored handler for given protocol {@param p}; if several
     * handlers can be found (for example, if exact protocol match failed, and
     * prefix+predicate search returned more than one handlers, the random one
     * will be invoked)
     * @param p handler for this protocol will be invoked
     * @param stream with this stream
     */
    virtual outcome::result<void> handle(
        const peer::ProtocolName &p,
        std::shared_ptr<connection::Stream> stream) = 0;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_ROUTER_HPP
