/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/impl/upgrader_session.hpp>

namespace libp2p::transport {

  UpgraderSession::UpgraderSession(
      std::shared_ptr<transport::Upgrader> upgrader, ProtoAddrVec layers,
      std::shared_ptr<connection::RawConnection> raw,
      UpgraderSession::HandlerFunc handler)
      : upgrader_(std::move(upgrader)),
        layers_(std::move(layers)),
        raw_(std::move(raw)),
        handler_(std::move(handler)) {}

  void UpgraderSession::upgradeInbound() {
    if (layers_.empty()) {
      return secureInbound(raw_);
    }

    auto on_layers_upgraded = [self{shared_from_this()}](auto &&res) {
      if (res.has_error()) {
        return self->handler_(res.as_failure());
      }
      auto &conn = res.value();
      self->secureInbound(std::move(conn));
    };

    upgrader_->upgradeLayersInbound(raw_, layers_,
                                    std::move(on_layers_upgraded));
  }

  void UpgraderSession::upgradeOutbound(const multi::Multiaddress &address,
                                        const peer::PeerId &remoteId) {
    if (layers_.empty()) {
      return secureOutbound(raw_, remoteId);
    }

    auto on_layers_upgraded = [self{shared_from_this()}, remoteId](auto &&res) {
      if (res.has_error()) {
        return self->handler_(res.as_failure());
      }
      auto &conn = res.value();
      self->secureOutbound(std::move(conn), remoteId);
    };

    upgrader_->upgradeLayersOutbound(address, raw_, layers_,
                                     std::move(on_layers_upgraded));
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

    upgrader_->upgradeToSecureOutbound(std::move(conn), remoteId,
                                       std::move(on_sec_upgraded));
  }

  void UpgraderSession::onSecured(
      outcome::result<std::shared_ptr<connection::SecureConnection>> rsecure) {
    if (!rsecure) {
      return handler_(rsecure.error());
    }

    upgrader_->upgradeToMuxed(
        rsecure.value(), [self{shared_from_this()}](auto &&res) {
          self->handler_(std::forward<decltype(res)>(res));
        });
  }

}  // namespace libp2p::transport
