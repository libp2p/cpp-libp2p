/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/tcp/tcp_transport.hpp>

#include <libp2p/transport/impl/upgrader_session.hpp>

namespace libp2p::transport {

  // Obtain host and port strings from provided address
  std::pair<std::string, std::string> getHostAndPort(
      const multi::Multiaddress &address) {
    auto v = address.getProtocolsWithValues();

    // get host
    auto it = v.begin();
    auto host = it->second;

    // get port
    it++;
    BOOST_ASSERT(it->first.code == multi::Protocol::Code::TCP);
    auto port = it->second;

    return {host, port};
  }

  void TcpTransport::dial(const peer::PeerId &remoteId,
                          multi::Multiaddress address,
                          TransportAdaptor::HandlerFunc handler) {
    if (!canDial(address)) {
      return handler(std::errc::address_family_not_supported);
    }

    auto conn = std::make_shared<TcpConnection>(*context_);

    auto [host, port] = getHostAndPort(address);

    conn->resolve(host, port,
                  [self{shared_from_this()}, conn, handler{std::move(handler)},
                   remoteId](auto ec, auto r) mutable {
                    if (ec) {
                      return handler(ec);
                    }

                    conn->connect(
                        r,
                        [self, conn, handler{std::move(handler)}, remoteId](
                            auto ec, auto &e) mutable {
                          if (ec) {
                            return handler(ec);
                          }

                          auto session = std::make_shared<UpgraderSession>(
                              self->upgrader_, std::move(conn), handler);

                          session->secureOutbound(remoteId);
                        });
                  });
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
