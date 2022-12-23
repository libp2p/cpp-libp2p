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
      if (!res) {
        return self->handler_(res.error());
      }
      self->secureInbound();
    };

    upgrader_->upgradeLayersInbound(raw_, std::move(on_layers_upgraded));
  }

  void UpgraderSession::upgradeOutbound(const peer::PeerId &remoteId) {
    auto on_layers_upgraded = [self{shared_from_this()}, remoteId](auto &&res) {
      if (!res) {
        return self->handler_(res.error());
      }
      self->secureOutbound(remoteId);
    };

    upgrader_->upgradeLayersOutbound(raw_, std::move(on_layers_upgraded));
  }

  void UpgraderSession::secureInbound() {
    auto on_sec_upgraded = [self{shared_from_this()}](auto &&res) {
      if (!res) {
        return self->handler_(res.error());
      }
      self->onSecured(res.value());
    };

    upgrader_->upgradeToSecureInbound(raw_, std::move(on_sec_upgraded));
  }

  void UpgraderSession::secureOutbound(const peer::PeerId &remoteId) {
    auto on_sec_upgraded = [self{shared_from_this()}](auto &&res) {
      if (!res) {
        return self->handler_(res.error());
      }
      self->onSecured(res.value());
    };

    upgrader_->upgradeToSecureOutbound(raw_, remoteId,
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
