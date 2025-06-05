/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/impl/upgrader_session.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace libp2p::transport {

  UpgraderSession::UpgraderSession(
      std::shared_ptr<transport::Upgrader> upgrader,
      ProtoAddrVec layers,
      std::shared_ptr<connection::RawConnection> raw,
      UpgraderSession::HandlerFunc handler)
      : upgrader_(std::move(upgrader)),
        layers_(std::move(layers)),
        raw_(std::move(raw)),
        handler_(std::move(handler)) {}

  void UpgraderSession::upgradeInbound() {
    auto on_layers_upgraded =
        [self{shared_from_this()}](
            outcome::result<std::shared_ptr<connection::LayerConnection>>
                res) {
          if (!res) {
            return self->handler_(res.as_failure());
          }
          auto &conn = res.value();
          self->secureInbound(std::move(conn));
        };

    upgrader_->upgradeLayersInbound(raw_, layers_, std::move(on_layers_upgraded));
  }

  boost::asio::awaitable<outcome::result<std::shared_ptr<connection::CapableConnection>>>
  UpgraderSession::upgradeInboundCoro() {
    // Step 1: Upgrade through layers
    auto layer_conn_res = co_await upgrader_->upgradeLayersInboundCoro(raw_, layers_);
    if (!layer_conn_res) {
      co_return layer_conn_res.error();
    }

    // Step 2: Upgrade to secure connection
    auto secure_conn_res = co_await secureInboundCoro(std::move(layer_conn_res.value()));
    if (!secure_conn_res) {
      co_return secure_conn_res.error();
    }

    // Step 3: Upgrade to muxed (capable) connection
    auto capable_conn_res = co_await onSecuredCoro(std::move(secure_conn_res.value()));
    co_return capable_conn_res;
  }

  void UpgraderSession::upgradeOutbound(const multi::Multiaddress &address,
                                        const peer::PeerId &remoteId) {
    auto on_layers_upgraded =
        [self{shared_from_this()}, remoteId](
            outcome::result<std::shared_ptr<connection::LayerConnection>>
                res) {
          if (!res) {
            return self->handler_(res.as_failure());
          }
          auto &conn = res.value();
          self->secureOutbound(std::move(conn), remoteId);
        };

    upgrader_->upgradeLayersOutbound(
        address, raw_, layers_, std::move(on_layers_upgraded));
  }

  boost::asio::awaitable<outcome::result<std::shared_ptr<connection::CapableConnection>>>
  UpgraderSession::upgradeOutboundCoro(const multi::Multiaddress &address,
                                      const peer::PeerId &remoteId) {
    // Step 1: Upgrade through layers
    auto layer_conn_res = co_await upgrader_->upgradeLayersOutboundCoro(address, raw_, layers_);
    if (!layer_conn_res) {
      co_return layer_conn_res.error();
    }

    // Step 2: Upgrade to secure connection
    auto secure_conn_res = co_await secureOutboundCoro(std::move(layer_conn_res.value()), remoteId);
    if (!secure_conn_res) {
      co_return secure_conn_res.error();
    }

    // Step 3: Upgrade to muxed (capable) connection
    auto capable_conn_res = co_await onSecuredCoro(std::move(secure_conn_res.value()));
    co_return capable_conn_res;
  }

  void UpgraderSession::secureInbound(
      std::shared_ptr<connection::LayerConnection> conn) {
    auto on_sec_upgraded = [self{shared_from_this()}](auto &&res) {
      if (!res) {
        return self->handler_(res.as_failure());
      }
      auto &conn = res.value();
      self->onSecured(std::move(conn));
    };

    upgrader_->upgradeToSecureInbound(std::move(conn),
                                      std::move(on_sec_upgraded));
  }

  void UpgraderSession::secureOutbound(
      std::shared_ptr<connection::LayerConnection> conn,
      const peer::PeerId &remoteId) {
    auto on_sec_upgraded = [self{shared_from_this()}](auto &&res) {
      if (!res) {
        return self->handler_(res.as_failure());
      }
      auto &conn = res.value();
      self->onSecured(std::move(conn));
    };

    upgrader_->upgradeToSecureOutbound(
        std::move(conn), remoteId, std::move(on_sec_upgraded));
  }

  boost::asio::awaitable<outcome::result<std::shared_ptr<connection::SecureConnection>>>
  UpgraderSession::secureInboundCoro(
      std::shared_ptr<connection::LayerConnection> conn) {
    auto secure_conn_res = co_await upgrader_->upgradeToSecureInboundCoro(std::move(conn));
    co_return secure_conn_res;
  }

  boost::asio::awaitable<outcome::result<std::shared_ptr<connection::SecureConnection>>>
  UpgraderSession::secureOutboundCoro(
      std::shared_ptr<connection::LayerConnection> conn,
      const peer::PeerId &remoteId) {
    auto secure_conn_res = co_await upgrader_->upgradeToSecureOutboundCoro(std::move(conn), remoteId);
    co_return secure_conn_res;
  }

  void UpgraderSession::onSecured(
      outcome::result<std::shared_ptr<connection::SecureConnection>> res) {
    if (!res) {
      return handler_(res.as_failure());
    }

    auto &secure_conn = res.value();

    auto on_muxed = [handler{handler_}](
                        outcome::result<std::shared_ptr<connection::CapableConnection>>
                            conn_res) mutable { handler(conn_res); };

    upgrader_->upgradeToMuxed(std::move(secure_conn), std::move(on_muxed));
  }

  boost::asio::awaitable<outcome::result<std::shared_ptr<connection::CapableConnection>>>
  UpgraderSession::onSecuredCoro(
      std::shared_ptr<connection::SecureConnection> secure_conn) {
    auto capable_conn_res = co_await upgrader_->upgradeToMuxedCoro(std::move(secure_conn));
    co_return capable_conn_res;
  }

}  // namespace libp2p::transport
