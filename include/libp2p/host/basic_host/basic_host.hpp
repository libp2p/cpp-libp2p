/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_HOST_HPP
#define LIBP2P_BASIC_HOST_HPP

#include <libp2p/event/bus.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/network/transport_manager.hpp>
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
              std::shared_ptr<event::Bus> bus,
              std::shared_ptr<network::TransportManager> transport_manager);

    std::string_view getLibp2pVersion() const override;

    std::string_view getLibp2pClientVersion() const override;

    peer::PeerId getId() const override;

    peer::PeerInfo getPeerInfo() const override;

    std::vector<multi::Multiaddress> getAddresses() const override;

    std::vector<multi::Multiaddress> getAddressesInterfaces() const override;

    std::vector<multi::Multiaddress> getObservedAddresses() const override;

    Connectedness connectedness(const peer::PeerInfo &p) const override;

    void connect(const peer::PeerInfo &peer_info,
                 const ConnectionResultHandler &handler,
                 std::chrono::milliseconds timeout) override;

    void disconnect(const peer::PeerId &peer_id) override;

    void setProtocolHandler(StreamProtocols protocols, StreamAndProtocolCb cb,
                            ProtocolPredicate predicate) override;

    void newStream(const peer::PeerInfo &peer_info, StreamProtocols protocols,
                   StreamAndProtocolOrErrorCb cb,
                   std::chrono::milliseconds timeout = {}) override;

    void newStream(const peer::PeerId &peer_id, StreamProtocols protocols,
                   StreamAndProtocolOrErrorCb cb) override;

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

    event::Handle setOnNewConnectionHandler(
        const NewConnectionHandler &h) const override;

   private:
    std::shared_ptr<peer::IdentityManager> idmgr_;
    std::unique_ptr<network::Network> network_;
    std::unique_ptr<peer::PeerRepository> repo_;
    std::shared_ptr<event::Bus> bus_;
    std::shared_ptr<network::TransportManager> transport_manager_;
  };

}  // namespace libp2p::host

#endif  // LIBP2P_BASIC_HOST_HPP
