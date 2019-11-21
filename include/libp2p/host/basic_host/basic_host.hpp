/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_HOST_HPP
#define LIBP2P_BASIC_HOST_HPP

#include <libp2p/event/bus.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/peer/identity_manager.hpp>

namespace libp2p::host {

  /**
   * @brief Basic host is a Host, which:
   * - has identity
   * - has access to a network
   * - has event bus
   * - has peer repository
   */
  class BasicHost : public Host {
   public:
    ~BasicHost() override = default;

    BasicHost(std::shared_ptr<peer::IdentityManager> idmgr,
              std::unique_ptr<network::Network> network,
              std::unique_ptr<peer::PeerRepository> repo,
              std::shared_ptr<event::Bus> bus);

    std::string_view getLibp2pVersion() const override;

    std::string_view getLibp2pClientVersion() const override;

    peer::PeerId getId() const override;

    peer::PeerInfo getPeerInfo() const override;

    std::vector<multi::Multiaddress> getAddresses() const override;

    std::vector<multi::Multiaddress> getAddressesInterfaces() const override;

    std::vector<multi::Multiaddress> getObservedAddresses() const override;

    void connect(const peer::PeerInfo &p) override;

    void setProtocolHandler(
        const peer::Protocol &proto,
        const std::function<connection::Stream::Handler> &handler) override;

    void setProtocolHandler(
        const peer::Protocol &proto,
        const std::function<connection::Stream::Handler> &handler,
        const std::function<bool(const peer::Protocol &)> &predicate) override;

    void newStream(const peer::PeerInfo &p, const peer::Protocol &protocol,
                   const StreamResultHandler &handler) override;

    outcome::result<void> listen(const multi::Multiaddress &ma) override;

    outcome::result<void> closeListener(const multi::Multiaddress &ma) override;

    outcome::result<void> removeListener(
        const multi::Multiaddress &ma) override;

    void start() override;

    void stop() override;

    network::Network &getNetwork() override;

    peer::PeerRepository &getPeerRepository() override;

    network::Router &getRouter() override;

    event::Bus &getBus() override;

   private:
    std::shared_ptr<peer::IdentityManager> idmgr_;
    std::unique_ptr<network::Network> network_;
    std::unique_ptr<peer::PeerRepository> repo_;
    std::shared_ptr<event::Bus> bus_;
  };

}  // namespace libp2p::host

#endif  // LIBP2P_BASIC_HOST_HPP
