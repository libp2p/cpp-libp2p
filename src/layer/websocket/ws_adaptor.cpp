/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_adaptor.hpp>

#include <libp2p/log/logger.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>

namespace libp2p::layer {

  WsAdaptor::WsAdaptor(std::shared_ptr<basic::Scheduler> scheduler,
                       std::shared_ptr<boost::asio::io_context> io_context,
                       WsConnectionConfig config)
      : scheduler_(std::move(scheduler)),
        io_context_(std::move(io_context)),
        config_(std::move(config)) {
    BOOST_ASSERT(scheduler_ != nullptr);
  }

  multi::Protocol::Code WsAdaptor::getProtocol() const {
    return multi::Protocol::Code::WS;
  }

  void WsAdaptor::upgradeInbound(
      std::shared_ptr<connection::LayerConnection> conn,
      LayerAdaptor::LayerConnCallbackFunc cb) const {
    log_->info("upgrade inbound connection to websocket");
    auto ws = std::make_shared<connection::WsConnection>(
        config_, io_context_, std::move(conn), scheduler_);
    ws->ws_.async_accept(
        [=, cb{std::move(cb)}](boost::system::error_code ec) mutable {
          if (ec) {
            return cb(ec);
          }
          ws->start();
          cb(std::move(ws));
        });
  }

  void WsAdaptor::upgradeOutbound(
      const multi::Multiaddress &address,
      std::shared_ptr<connection::LayerConnection> conn,
      LayerAdaptor::LayerConnCallbackFunc cb) const {
    auto host = transport::detail::getHostAndTcpPort(address).first;
    auto ws = std::make_shared<connection::WsConnection>(
        config_, io_context_, std::move(conn), scheduler_);
    ws->ws_.async_handshake(
        host, "/",
        [=, cb{std::move(cb)}](boost::system::error_code ec) mutable {
          if (ec) {
            return cb(ec);
          }
          ws->start();
          cb(std::move(ws));
        });
  }

}  // namespace libp2p::layer
