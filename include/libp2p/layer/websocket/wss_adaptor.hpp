/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LAYER_WEBSOCKET_WSS_ADAPTOR_HPP
#define LIBP2P_LAYER_WEBSOCKET_WSS_ADAPTOR_HPP

#include <libp2p/layer/layer_adaptor.hpp>

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace boost::asio::ssl {
  class context;
}  // namespace boost::asio::ssl

namespace libp2p::layer {
  class WsAdaptor;

  struct WssCertificate {
    static outcome::result<WssCertificate> make(std::string_view pem);

    std::shared_ptr<boost::asio::ssl::context> context;
  };

  class WssAdaptor : public LayerAdaptor {
   public:
    WssAdaptor(std::shared_ptr<boost::asio::io_context> io_context,
               WssCertificate certificate,
               std::shared_ptr<WsAdaptor> ws_adaptor);

    multi::Protocol::Code getProtocol() const override;

    void upgradeInbound(std::shared_ptr<connection::LayerConnection> conn,
                        LayerConnCallbackFunc cb) const override;

    void upgradeOutbound(const multi::Multiaddress &address,
                         std::shared_ptr<connection::LayerConnection> conn,
                         LayerConnCallbackFunc cb) const override;

   private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    WssCertificate server_certificate_;
    std::shared_ptr<boost::asio::ssl::context> client_context_;
    std::shared_ptr<WsAdaptor> ws_adaptor_;
  };
}  // namespace libp2p::layer

#endif  // LIBP2P_LAYER_WEBSOCKET_WSS_ADAPTOR_HPP
