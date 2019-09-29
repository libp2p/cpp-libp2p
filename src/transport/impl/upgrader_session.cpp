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

  void UpgraderSession::secureOutbound(const peer::PeerId &remoteId) {
    auto self{shared_from_this()};
    upgrader_->upgradeToSecureOutbound(raw_, remoteId, [self](auto &&r) {
      self->onSecured(std::forward<decltype(r)>(r));
    });
  }

  void UpgraderSession::secureInbound() {
    upgrader_->upgradeToSecureInbound(
        raw_, [self{shared_from_this()}](auto &&r) {
          self->onSecured(std::forward<decltype(r)>(r));
        });
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
