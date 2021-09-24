/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_NETWORK_IMPL_HPP
#define LIBP2P_NETWORK_IMPL_HPP

#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/network.hpp>

namespace libp2p::network {

  class NetworkImpl : public Network {
   public:
    ~NetworkImpl() override = default;

    NetworkImpl(std::shared_ptr<ListenerManager> listener,
                std::shared_ptr<Dialer> dialer,
                std::shared_ptr<ConnectionManager> cmgr);

    void closeConnections(const peer::PeerId &p) override;

    Dialer &getDialer() override;

    ListenerManager &getListener() override;

    ConnectionManager &getConnectionManager() override;

   private:
    std::shared_ptr<ListenerManager> listener_;
    std::shared_ptr<Dialer> dialer_;
    std::shared_ptr<ConnectionManager> cmgr_;
  };

}  // namespace libp2p::network

#endif  // LIBP2P_NETWORK_IMPL_HPP
