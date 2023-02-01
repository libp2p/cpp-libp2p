/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/wss_adaptor.hpp>

#include <libp2p/layer/websocket/ssl_connection.hpp>
#include <libp2p/layer/websocket/ws_adaptor.hpp>

namespace libp2p::layer {
  outcome::result<WssCertificate> WssCertificate::make(std::string_view pem) {
    auto context = std::make_shared<boost::asio::ssl::context>(
        boost::asio::ssl::context::tls);
    boost::system::error_code ec;
    context->use_private_key(boost::asio::buffer(pem),
                             boost::asio::ssl::context::file_format::pem, ec);
    if (ec) {
      return ec;
    }
    context->use_certificate_chain(boost::asio::buffer(pem), ec);
    if (ec) {
      return ec;
    }
    return WssCertificate{context};
  }

  WssAdaptor::WssAdaptor(std::shared_ptr<boost::asio::io_context> io_context,
                         WssCertificate certificate,
                         std::shared_ptr<WsAdaptor> ws_adaptor)
      : io_context_{std::move(io_context)},
        server_certificate_{std::move(certificate)},
        client_context_{std::make_shared<boost::asio::ssl::context>(
            boost::asio::ssl::context::tls)},
        ws_adaptor_{std::move(ws_adaptor)} {}

  multi::Protocol::Code WssAdaptor::getProtocol() const {
    return multi::Protocol::Code::WSS;
  }

  void WssAdaptor::upgradeInbound(
      std::shared_ptr<connection::LayerConnection> conn,
      LayerAdaptor::LayerConnCallbackFunc cb) const {
    if (not server_certificate_.context) {
      return cb(std::errc::address_family_not_supported);
    }
    auto ssl = std::make_shared<connection::SslConnection>(
        io_context_, std::move(conn), server_certificate_.context);
    ssl->ssl_.async_handshake(
        boost::asio::ssl::stream_base::handshake_type::server,
        [=, ws{ws_adaptor_},
         cb{std::move(cb)}](boost::system::error_code ec) mutable {
          if (ec) {
            return cb(ec);
          }
          ws->upgradeInbound(std::move(ssl), std::move(cb));
        });
  }

  void WssAdaptor::upgradeOutbound(
      const multi::Multiaddress &address,
      std::shared_ptr<connection::LayerConnection> conn,
      LayerAdaptor::LayerConnCallbackFunc cb) const {
    auto ssl = std::make_shared<connection::SslConnection>(
        io_context_, std::move(conn), client_context_);
    ssl->ssl_.async_handshake(
        boost::asio::ssl::stream_base::handshake_type::client,
        [=, ws{ws_adaptor_},
         cb{std::move(cb)}](boost::system::error_code ec) mutable {
          if (ec) {
            return cb(ec);
          }
          ws->upgradeOutbound(address, std::move(ssl), std::move(cb));
        });
  }
}  // namespace libp2p::layer
