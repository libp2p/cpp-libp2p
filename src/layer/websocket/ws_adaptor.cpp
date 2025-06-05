/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_adaptor.hpp>

#include <libp2p/log/logger.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>
#include <boost/asio/use_awaitable.hpp>

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

  boost::asio::awaitable<outcome::result<std::shared_ptr<connection::LayerConnection>>>
  WsAdaptor::upgradeInbound(std::shared_ptr<connection::LayerConnection> conn) const {
    log_->info("upgrade inbound connection to websocket (coroutine)");
    auto ws = std::make_shared<connection::WsConnection>(
        config_, io_context_, std::move(conn), scheduler_);

    try {
      co_await ws->ws_.async_accept(boost::asio::use_awaitable);
      ws->start();
      co_return outcome::success(std::move(ws));
    } catch (const boost::system::system_error &error) {
      co_return outcome::failure(error.code());
    } catch (const std::exception &) {
      co_return outcome::failure(std::errc::io_error);
    }
  }

  void WsAdaptor::upgradeOutbound(
      const multi::Multiaddress &address,
      std::shared_ptr<connection::LayerConnection> conn,
      LayerAdaptor::LayerConnCallbackFunc cb) const {
    auto host = address.getProtocolsWithValues().begin()->second;
    auto ws = std::make_shared<connection::WsConnection>(
        config_, io_context_, std::move(conn), scheduler_);
    ws->ws_.async_handshake(
        host,
        "/",
        [=, cb{std::move(cb)}](boost::system::error_code ec) mutable {
          if (ec) {
            return cb(ec);
          }
          ws->start();
          cb(std::move(ws));
        });
  }

  boost::asio::awaitable<outcome::result<std::shared_ptr<connection::LayerConnection>>>
  WsAdaptor::upgradeOutbound(
      const multi::Multiaddress &address,
      std::shared_ptr<connection::LayerConnection> conn) const {
    auto host = address.getProtocolsWithValues().begin()->second;
    auto ws = std::make_shared<connection::WsConnection>(
        config_, io_context_, std::move(conn), scheduler_);

    try {
      co_await ws->ws_.async_handshake(
          host,
          "/",
          boost::asio::use_awaitable);
      ws->start();
      co_return outcome::success(std::move(ws));
    } catch (const boost::system::system_error &error) {
      co_return outcome::failure(error.code());
    } catch (const std::exception &) {
      co_return outcome::failure(std::errc::io_error);
    }
  }

}  // namespace libp2p::layer
