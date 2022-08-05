/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LISTENER_MANAGER_IMPL_HPP
#define LIBP2P_LISTENER_MANAGER_IMPL_HPP

#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/listener_manager.hpp>
#include <libp2p/network/transport_manager.hpp>
#include <libp2p/peer/address_repository.hpp>
#include <libp2p/protocol_muxer/protocol_muxer.hpp>

namespace libp2p::network {

  class ListenerManagerImpl : public ListenerManager {
   public:
    ~ListenerManagerImpl() override = default;

    ListenerManagerImpl(
        std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect,
        std::shared_ptr<Router> router, std::shared_ptr<TransportManager> tmgr,
        std::shared_ptr<ConnectionManager> cmgr);

    bool isStarted() const override;

    void start() override;

    void stop() override;

    outcome::result<void> closeListener(const multi::Multiaddress &ma) override;

    outcome::result<void> removeListener(
        const multi::Multiaddress &ma) override;

    outcome::result<void> listen(const multi::Multiaddress &ma) override;

    std::vector<multi::Multiaddress> getListenAddresses() const override;

    std::vector<multi::Multiaddress> getListenAddressesInterfaces()
        const override;

    Router &getRouter() override;

    void onConnection(
        outcome::result<std::shared_ptr<connection::CapableConnection>> rconn)
        override;

   private:
    bool started = false;

    // clang-format off
    std::unordered_map<multi::Multiaddress, std::shared_ptr<transport::TransportListener>> listeners_;
    // clang-format on

    std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<TransportManager> tmgr_;
    std::shared_ptr<ConnectionManager> cmgr_;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_LISTENER_MANAGER_IMPL_HPP
