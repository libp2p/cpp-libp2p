/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_NETWORK_LISTENER_HPP
#define LIBP2P_NETWORK_LISTENER_HPP

#include <vector>

#include <libp2p/event/bus.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/network/router.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/base_protocol.hpp>

namespace libp2p::network {

  namespace event {
    using libp2p::event::channel_decl;

    struct ListenAddressAdded {};
    using ListenAddressAddedChannel =
        channel_decl<ListenAddressAdded, multi::Multiaddress>;

    struct ListenAddressRemoved {};
    using ListenAddressRemovedChannel =
        channel_decl<ListenAddressRemoved, multi::Multiaddress>;
  }  // namespace event

  /**
   * @brief Class, which is capable of listening (opening a server) on
   * registered transports.
   */
  struct ListenerManager {
    using StreamResult = outcome::result<std::shared_ptr<connection::Stream>>;
    using StreamResultFunc = std::function<void(StreamResult)>;

    virtual ~ListenerManager() = default;

    /**
     * @brief Returns true if listener started listening.
     */
    virtual bool isStarted() const = 0;  // NOLINT(modernize-use-nodiscard)

    /**
     * @brief Start all listeners on supplied multiaddresses
     */
    virtual void start() = 0;

    /**
     * @brief Stop listening on all multiaddresses. Does not delete existing
     * listeners.
     */
    virtual void stop() = 0;

    /**
     * @brief Close (but don't remove) listener and all incoming connections on
     * address \ma
     * @param ma address to be closed
     * @return error if close failed or no listener with given \ma exist
     */
    virtual outcome::result<void> closeListener(
        const multi::Multiaddress &ma) = 0;

    /**
     * @brief Close, then remove listener and all incoming connection on address
     * \ma
     * @param ma address to be closed
     * @return error if close failed or no listener with given \ma exist
     */
    virtual outcome::result<void> removeListener(
        const multi::Multiaddress &ma) = 0;

    /**
     * @brief Listen tells the ListenerManager to start listening on given
     * multiaddr. May be executed many times (with different
     * addresses/protocols).
     * @param ma address to be used for listener
     * @return
     */
    virtual outcome::result<void> listen(const multi::Multiaddress &ma) = 0;

    /**
     * @brief Returns an unmodified list of addresses, added by user.
     */
    virtual std::vector<multi::Multiaddress> getListenAddresses() const = 0;

    /**
     * @brief Returns all interface addressess we are listening on. May be
     * different from those supplied to `listen`.
     *
     * @example: /ip4/0.0.0.0/tcp/0 -> /ip4/0.0.0.0/tcp/54211 (random port)
     */
    virtual std::vector<multi::Multiaddress> getListenAddressesInterfaces()
        const = 0;

    /**
     * @brief Add new protocol handler
     * @param protocol protocol to be handled
     * @param cb callback executed on new stream
     */
    virtual void setProtocolHandler(const peer::Protocol &protocol,
                                    StreamResultFunc cb) = 0;

    /**
     * @brief Add new protocol handler with matcher
     * @param protocol protocol to be handled
     * @param cb callback executed on new stream
     * @param matcher function, which is executed to match protocols.
     */
    virtual void setProtocolHandler(const peer::Protocol &protocol,
                                    StreamResultFunc cb,
                                    Router::ProtoPredicate matcher) = 0;

    /**
     * @brief Getter for Router.
     */
    virtual Router &getRouter() = 0;

    /**
     * @brief Allows new connections for accepting incoming streams
     */
    virtual void onConnection(
        outcome::result<std::shared_ptr<connection::CapableConnection>>
            rconn) = 0;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_NETWORK_LISTENER_HPP
