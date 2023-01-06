/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LAYER_WEBSOCKETADAPTOR
#define LIBP2P_LAYER_WEBSOCKETADAPTOR

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/crypto/random_generator.hpp>
#include <libp2p/layer/layer_adaptor.hpp>
#include <libp2p/layer/layer_connection_config.hpp>
#include <libp2p/layer/websocket/ws_connection.hpp>
#include <libp2p/network/connection_manager.hpp>

namespace libp2p::layer {

  class WsAdaptor : public LayerAdaptor {
   public:
    static constexpr auto kNoSecureProtocolId = "/ws";
    static constexpr auto kSecureProtocolId = "/wss";

    WsAdaptor(std::shared_ptr<basic::Scheduler> scheduler,
              std::shared_ptr<crypto::random::RandomGenerator> random_generator,
              std::shared_ptr<const WsConnectionConfig> config,
              bool tls_enabled = false);

    ~WsAdaptor() override = default;

    peer::Protocol getProtocolId() const noexcept override {
      return tls_enabled_ ? kSecureProtocolId : kNoSecureProtocolId;
    }

    void upgradeInbound(std::shared_ptr<connection::LayerConnection> conn,
                        LayerConnCallbackFunc cb) const override;

    void upgradeOutbound(std::shared_ptr<connection::LayerConnection> conn,
                         LayerConnCallbackFunc cb) const override;

   private:
    std::shared_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<crypto::random::RandomGenerator> random_generator_;
    std::shared_ptr<const WsConnectionConfig> config_;
    bool tls_enabled_;

    log::Logger log_ = log::createLogger("WsAdaptor");
  };

}  // namespace libp2p::layer

#endif  // LIBP2P_LAYER_WEBSOCKETADAPTOR
