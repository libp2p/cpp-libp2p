/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LAYER_WEBSOCKETADAPTOR
#define LIBP2P_LAYER_WEBSOCKETADAPTOR

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/layer/layer_adaptor.hpp>
#include <libp2p/layer/websocket/ws_connection.hpp>
#include <libp2p/layer/websocket/ws_connection_config.hpp>
#include <libp2p/network/connection_manager.hpp>

namespace libp2p::layer {

  class WsAdaptor : public LayerAdaptor {
   public:
    WsAdaptor(std::shared_ptr<basic::Scheduler> scheduler,
              std::shared_ptr<boost::asio::io_context> io_context,
              WsConnectionConfig config);

    multi::Protocol::Code getProtocol() const override;

    void upgradeInbound(std::shared_ptr<connection::LayerConnection> conn,
                        LayerConnCallbackFunc cb) const override;

    void upgradeOutbound(const multi::Multiaddress &address,
                         std::shared_ptr<connection::LayerConnection> conn,
                         LayerConnCallbackFunc cb) const override;

   private:
    std::shared_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    WsConnectionConfig config_;

    log::Logger log_ = log::createLogger("WsAdaptor");
  };

}  // namespace libp2p::layer

#endif  // LIBP2P_LAYER_WEBSOCKETADAPTOR
