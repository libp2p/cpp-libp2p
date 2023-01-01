/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_adaptor.hpp>

#include <libp2p/layer/websocket/http_to_ws_upgrader.hpp>
#include <libp2p/log/logger.hpp>

namespace libp2p::layer {

  WsAdaptor::WsAdaptor(
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<crypto::hmac::HmacProvider> hmac_provider,
      std::shared_ptr<const WsConnectionConfig> config, bool tls_enabled)
      : scheduler_(std::move(scheduler)),
        hmac_provider_{std::move(hmac_provider)},
        config_(std::move(config)),
        tls_enabled_(tls_enabled) {
    BOOST_ASSERT(scheduler_ != nullptr);
  }

  void WsAdaptor::upgradeInbound(
      std::shared_ptr<connection::LayerConnection> conn,
      LayerAdaptor::LayerConnCallbackFunc cb) const {
    log_->info("upgrade inbound connection to websocket");
    auto upgrader = std::make_shared<websocket::HttpToWsUpgrader>(
        conn, false, std::move(cb), scheduler_, config_, hmac_provider_);
    upgrader->upgrade();
  }

  void WsAdaptor::upgradeOutbound(
      std::shared_ptr<connection::LayerConnection> conn,
      LayerAdaptor::LayerConnCallbackFunc cb) const {
    log_->info("upgrade outbound connection to websocket");
    auto upgrader = std::make_shared<websocket::HttpToWsUpgrader>(
        conn, true, std::move(cb), scheduler_, config_, hmac_provider_);
    upgrader->upgrade();
  }

}  // namespace libp2p::layer