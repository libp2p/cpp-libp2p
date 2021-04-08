/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/tcp/tcp_transport.hpp>

#include <libp2p/transport/impl/upgrader_session.hpp>

namespace libp2p::transport {

  void TcpTransport::dial(const peer::PeerId &remoteId,
                          multi::Multiaddress address,
                          TransportAdaptor::HandlerFunc handler) {
    dial(remoteId, std::move(address), std::move(handler),
         std::chrono::milliseconds::zero());
  }

  void TcpTransport::dial(const peer::PeerId &remoteId,
                          multi::Multiaddress address,
                          TransportAdaptor::HandlerFunc handler,
                          std::chrono::milliseconds timeout) {
    if (!canDial(address)) {
      //TODO(107): Reentrancy

      return handler(std::errc::address_family_not_supported);
    }

    auto conn = std::make_shared<TcpConnection>(*context_);

    auto [host, port] = detail::getHostAndTcpPort(address);

    auto connect = [self{shared_from_this()}, conn, handler{std::move(handler)},
                    remoteId, timeout](auto ec, auto r) mutable {
      if (ec) {
        return handler(ec);
      }

      conn->connect(
          r,
          [self, conn, handler{std::move(handler)}, remoteId](auto ec,
                                                              auto &e) mutable {
            if (ec) {
              return handler(ec);
            }

            auto session = std::make_shared<UpgraderSession>(
                self->upgrader_, std::move(conn), handler);

            session->secureOutbound(remoteId);
          },
          timeout);
    };

    using P = multi::Protocol::Code;
    switch (detail::getFirstProtocol(address)) {
      case P::DNS4:
        return conn->resolve(boost::asio::ip::tcp::v4(), host, port, connect);
      case P::DNS6:
        return conn->resolve(boost::asio::ip::tcp::v6(), host, port, connect);
      default:  // Could be only DNS, IP6 or IP4 as canDial already checked for
                // that in the beginning of the method
        return conn->resolve(host, port, connect);
    }
  }

  std::shared_ptr<TransportListener> TcpTransport::createListener(
      TransportListener::HandlerFunc handler) {
    return std::make_shared<TcpListener>(*context_, upgrader_,
                                         std::move(handler));
  }

  bool TcpTransport::canDial(const multi::Multiaddress &ma) const {
    return detail::supportsIpTcp(ma);
  }

  TcpTransport::TcpTransport(std::shared_ptr<boost::asio::io_context> context,
                             std::shared_ptr<Upgrader> upgrader)
      : context_(std::move(context)), upgrader_(std::move(upgrader)) {}

  peer::Protocol TcpTransport::getProtocolId() const {
    return "/tcp/1.0.0";
  }

}  // namespace libp2p::transport
