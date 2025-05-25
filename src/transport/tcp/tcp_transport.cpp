/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/tcp/tcp_transport.hpp>

#include <libp2p/transport/impl/upgrader_session.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>

namespace libp2p::transport {

  void TcpTransport::dial(const peer::PeerId &remoteId,
                          multi::Multiaddress address,
                          TransportAdaptor::HandlerFunc handler) {
    auto r = detail::asTcp(address);
    if (not r) {
      return handler(r.error());
    }
    auto &[info, layers] = r.value();
    auto conn = std::make_shared<TcpConnection>(*context_, layers);
    auto connect =
        [weak{weak_from_this()},
         handler{std::move(handler)},
         layers = std::move(layers),
         conn = std::move(conn),
         address,
         remoteId](outcome::result<boost::asio::ip::tcp::resolver::results_type>
                       r) mutable {
          if (not r) {
            return handler(r.error());
          }

          auto self = weak.lock();
          if (not self) {
            return;
          }

          conn->connect(
              r.value(),
              [weak{self->weak_from_this()},
               handler{std::move(handler)},
               layers = std::move(layers),
               conn = std::move(conn),
               address,
               remoteId](auto ec, auto &e) mutable {
                auto self = weak.lock();
                if (not self) {
                  return;
                }
                if (ec) {
                  std::ignore = conn->close();
                  return handler(ec);
                }

                auto session =
                    std::make_shared<UpgraderSession>(self->upgrader_,
                                                      std::move(layers),
                                                      std::move(conn),
                                                      handler);

                session->upgradeOutbound(address, remoteId);
              },
              self->mux_config_.dial_timeout);
        };
    resolve(resolver_, info, std::move(connect), mux_config_.dial_timeout);
  }

  std::shared_ptr<TransportListener> TcpTransport::createListener(
      TransportListener::HandlerFunc handler) {
    return std::make_shared<TcpListener>(
        *context_, upgrader_, std::move(handler));
  }

  bool TcpTransport::canDial(const multi::Multiaddress &ma) const {
    return detail::asTcp(ma).has_value();
  }

  TcpTransport::TcpTransport(std::shared_ptr<boost::asio::io_context> context,
                             const muxer::MuxedConnectionConfig &mux_config,
                             std::shared_ptr<Upgrader> upgrader)
      : context_{std::move(context)},
        mux_config_{mux_config},
        upgrader_{std::move(upgrader)},
        resolver_{*context_} {}

  peer::ProtocolName TcpTransport::getProtocolId() const {
    return "/tcp/1.0.0";
  }

}  // namespace libp2p::transport
