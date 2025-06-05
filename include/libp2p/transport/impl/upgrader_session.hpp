/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <boost/asio/awaitable.hpp>
#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/transport/upgrader.hpp>

namespace libp2p::transport {

  /**
   * @brief Class, which reduces callback hell in transport upgrader.
   */
  struct UpgraderSession
      : public std::enable_shared_from_this<UpgraderSession> {
    using ConnectionCallback =
        void(outcome::result<std::shared_ptr<connection::CapableConnection>>);
    using HandlerFunc = std::function<ConnectionCallback>;

    UpgraderSession(std::shared_ptr<transport::Upgrader> upgrader,
                    ProtoAddrVec layers,
                    std::shared_ptr<connection::RawConnection> raw,
                    HandlerFunc handler);

    void upgradeInbound();

    void upgradeOutbound(const multi::Multiaddress &address,
                         const peer::PeerId &remoteId);

    /**
     * Coroutine version of upgradeInbound
     * @return awaitable with capable connection or error
     */
    boost::asio::awaitable<
        outcome::result<std::shared_ptr<connection::CapableConnection>>>
    upgradeInboundCoro();

    /**
     * Coroutine version of upgradeOutbound
     * @param address - multiaddress to connect to
     * @param remoteId - remote peer ID
     * @return awaitable with capable connection or error
     */
    boost::asio::awaitable<
        outcome::result<std::shared_ptr<connection::CapableConnection>>>
    upgradeOutboundCoro(const multi::Multiaddress &address,
                       const peer::PeerId &remoteId);

   private:
    std::shared_ptr<transport::Upgrader> upgrader_;
    ProtoAddrVec layers_;
    std::shared_ptr<connection::RawConnection> raw_;
    HandlerFunc handler_;

    void secureOutbound(std::shared_ptr<connection::LayerConnection> conn,
                        const peer::PeerId &remoteId);

    void secureInbound(std::shared_ptr<connection::LayerConnection> conn);

    /**
     * Coroutine version of secureInbound
     * @param conn - connection to be upgraded
     * @return awaitable with secured connection or error
     */
    boost::asio::awaitable<
        outcome::result<std::shared_ptr<connection::SecureConnection>>>
    secureInboundCoro(std::shared_ptr<connection::LayerConnection> conn);

    /**
     * Coroutine version of secureOutbound
     * @param conn - connection to be upgraded
     * @param remoteId - remote peer ID
     * @return awaitable with secured connection or error
     */
    boost::asio::awaitable<
        outcome::result<std::shared_ptr<connection::SecureConnection>>>
    secureOutboundCoro(std::shared_ptr<connection::LayerConnection> conn,
                      const peer::PeerId &remoteId);

    void onSecured(
        outcome::result<std::shared_ptr<connection::SecureConnection>> res);

    /**
     * Coroutine version of onSecured
     * @param secure_conn - secure connection to be muxed
     * @return awaitable with capable connection or error
     */
    boost::asio::awaitable<
        outcome::result<std::shared_ptr<connection::CapableConnection>>>
    onSecuredCoro(std::shared_ptr<connection::SecureConnection> secure_conn);

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(
        libp2p::transport::UpgraderSession);
  };

}  // namespace libp2p::transport
