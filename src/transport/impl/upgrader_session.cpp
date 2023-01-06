/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/impl/upgrader_session.hpp>

namespace libp2p::transport {

  UpgraderSession::UpgraderSession(
      std::shared_ptr<transport::Upgrader> upgrader,
      std::shared_ptr<connection::RawConnection> raw,
      UpgraderSession::HandlerFunc handler)
      : upgrader_(std::move(upgrader)),
        raw_(std::move(raw)),
        handler_(std::move(handler)) {}

  void UpgraderSession::upgradeInbound() {
    auto on_layers_upgraded = [self{shared_from_this()}](auto &&res) {
      if (res.has_error()) {
        return self->handler_(res.as_failure());
      }
      auto &conn = res.value();
      self->secureInbound(std::move(conn));
    };

    upgrader_->upgradeLayersInbound(raw_, std::move(on_layers_upgraded));
  }

  void UpgraderSession::upgradeOutbound(std::string layers,
                                        const peer::PeerId &remoteId) {
    if (layers.empty()) {
      return secureOutbound(raw_, remoteId);
    }

    auto on_layers_upgraded = [self{shared_from_this()}, remoteId](auto &&res) {
      if (res.has_error()) {
        return self->handler_(res.as_failure());
      }
      auto &conn = res.value();
      self->secureOutbound(std::move(conn), remoteId);
    };

    upgrader_->upgradeLayersOutbound(raw_, layers,
                                     std::move(on_layers_upgraded));
  }

  void UpgraderSession::secureInbound(
      std::shared_ptr<connection::LayerConnection> conn) {
    auto on_sec_upgraded = [self{shared_from_this()}](auto &&res) {
      if (!res) {
        return self->handler_(res.as_failure());
      }
      auto &conn = res.value();
      self->onSecured(res.value());
    };

    upgrader_->upgradeToSecureInbound(conn, std::move(on_sec_upgraded));
  }

  void UpgraderSession::secureOutbound(
      std::shared_ptr<connection::LayerConnection> conn,
      const peer::PeerId &remoteId) {
    auto on_sec_upgraded = [self{shared_from_this()}](auto &&res) {
      if (!res) {
        return self->handler_(res.as_failure());
      }
      self->onSecured(res.value());
    };

    upgrader_->upgradeToSecureOutbound(conn, remoteId,
                                       std::move(on_sec_upgraded));
  }

  void UpgraderSession::onSecured(
      outcome::result<std::shared_ptr<connection::SecureConnection>> rsecure) {
    if (!rsecure) {
      return handler_(rsecure.error());
    }

    upgrader_->upgradeToMuxed(rsecure.value(),
                              [self{shared_from_this()}](auto &&r) {
                                self->handler_(std::forward<decltype(r)>(r));
                              });
  }

}  // namespace libp2p::transport
