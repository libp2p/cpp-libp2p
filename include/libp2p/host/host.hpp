/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_HOST_HPP
#define LIBP2P_HOST_HPP

#include <functional>
#include <string_view>

#include <libp2p/connection/stream.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/network/network.hpp>
#include <libp2p/network/router.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/peer_repository.hpp>
#include <libp2p/peer/protocol.hpp>
#include <libp2p/protocol/base_protocol.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p {
  /**
   * Main class, which represents single peer in p2p network.
   *
   * It is capable of:
   * - create new connections to remote peers
   * - create new streams to remote peers
   * - listen on one or multiple addresses
   * - register protocols
   * - handle registered protocols (receive and handle incoming streams with
   * given protocol)
   */
  struct Host {
    virtual ~Host() = default;

    using StreamResultHandler = std::function<void(
        outcome::result<std::shared_ptr<connection::Stream>>)>;

    /**
     * @brief Get a version of Libp2p, supported by this Host
     */
    virtual std::string_view getLibp2pVersion() const = 0;

    /**
     * @brief Get a version of this Libp2p client
     */
    virtual std::string_view getLibp2pClientVersion() const = 0;

    /**
     * @brief Get identifier of this Host
     */
    virtual peer::PeerId getId() const = 0;

    /**
     * @brief Get PeerInfo of this Host
     */
    virtual peer::PeerInfo getPeerInfo() const = 0;

    /**
     * @brief Get addresses we provided to listen on (added by listen).
     */
    virtual std::vector<multi::Multiaddress> getAddresses() const = 0;

    /**
     * @brief Get addresses that read from listen sockets.
     *
     * May return 0 addresses if no listeners found or all listeners stopped.
     */
    virtual std::vector<multi::Multiaddress> getAddressesInterfaces() const = 0;

    /**
     * @brief Get our addresses observed by other peers.
     *
     * May return 0 addresses if we don't know our observed addresses.
     */
    virtual std::vector<multi::Multiaddress> getObservedAddresses() const = 0;

    /**
     * @brief Let Host handle given {@param proto} protocol
     * @param proto protocol to be handled
     * @param handler callback that is executed when some other Host creates
     * stream to our host with {@param proto} protocol.
     */
    virtual void setProtocolHandler(
        const peer::Protocol &proto,
        const std::function<connection::Stream::Handler> &handler) = 0;

    /**
     * @brief Let Host handle given protocol, and use matcher to check if we
     * support given remote protocol.
     * @param handler of the arrived stream
     * @param predicate function that takes received protocol (/ping/1.0.0) and
     * should return true, if this protocol can be handled.
     */
    virtual void setProtocolHandler(
        const peer::Protocol &protocol,
        const std::function<connection::Stream::Handler> &handler,
        const std::function<bool(const peer::Protocol &)> &predicate) = 0;

    /**
     * @brief Initiates connection to the peer {@param p}. If connection exists,
     * does nothing.
     * @param p peer to connect.
     */
    virtual void connect(const peer::PeerInfo &p) = 0;

    /**
     * @brief Open new stream to the peer {@param p} with protocol {@param
     * protocol}.
     * @param p stream will be opened to this peer
     * @param protocol "speak" using this protocol
     * @param handler callback, will be executed on success or fail
     */
    virtual void newStream(const peer::PeerInfo &p,
                           const peer::Protocol &protocol,
                           const StreamResultHandler &handler) = 0;

    /**
     * @brief Create listener on given multiaddress.
     * @param ma address
     * @return may return error
     */
    virtual outcome::result<void> listen(const multi::Multiaddress &ma) = 0;

    /**
     * @brief Close listener on given address.
     * @param ma address
     * @return may return error
     */
    virtual outcome::result<void> closeListener(
        const multi::Multiaddress &ma) = 0;

    /**
     * @brief Removes listener on given address.
     * @param ma address
     * @return may return error
     */
    virtual outcome::result<void> removeListener(
        const multi::Multiaddress &ma) = 0;

    /**
     * @brief Start all listeners.
     */
    virtual void start() = 0;

    /**
     * @brief Stop all listeners.
     */
    virtual void stop() = 0;

    /**
     * @brief Getter for a network.
     */
    virtual network::Network &getNetwork() = 0;

    /**
     * @brief Getter for a peer repository.
     */
    virtual peer::PeerRepository &getPeerRepository() = 0;

    /**
     * @brief Getter for a router.
     */
    virtual network::Router &getRouter() = 0;

    /**
     * @brief Getter for event bus.
     */
    virtual event::Bus &getBus() = 0;
  };
}  // namespace libp2p

#endif  // LIBP2P_HOST_HPP
