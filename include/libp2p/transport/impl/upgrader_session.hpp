/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_UPGRADER_SESSION_HPP
#define LIBP2P_UPGRADER_SESSION_HPP

#include <memory>

#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/transport/upgrader.hpp>

namespace libp2p::transport {

  /**
   * @brief Class, which reduces callback hell in transport upgrader.
   */
  struct UpgraderSession
      : public std::enable_shared_from_this<UpgraderSession> {
    using ProtoAddrVec = std::vector<std::pair<multi::Protocol, std::string>>;
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

   private:
    std::shared_ptr<transport::Upgrader> upgrader_;
    ProtoAddrVec layers_;
    std::shared_ptr<connection::RawConnection> raw_;
    HandlerFunc handler_;

    void secureOutbound(std::shared_ptr<connection::LayerConnection> conn,
                        const peer::PeerId &remoteId);

    void secureInbound(std::shared_ptr<connection::LayerConnection> conn);

    void onSecured(
        outcome::result<std::shared_ptr<connection::SecureConnection>> res);

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(
        libp2p::transport::UpgraderSession);
  };

}  // namespace libp2p::transport

#endif  // LIBP2P_UPGRADER_SESSION_HPP
