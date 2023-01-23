/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LAYER_WEBSOCKET_HTTPTOWSUPGRADER
#define LIBP2P_LAYER_WEBSOCKET_HTTPTOWSUPGRADER

#include <libp2p/basic/message_read_writer.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/hmac_provider.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/crypto/random_generator.hpp>
#include <libp2p/crypto/x25519_provider.hpp>
#include <libp2p/layer/layer_adaptor.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "http_read_writer.hpp"
#include "ws_connection_config.hpp"

namespace libp2p::layer::websocket {

  class HttpToWsUpgrader
      : public std::enable_shared_from_this<HttpToWsUpgrader> {
   public:
    static constexpr size_t kMaxMsgLen = 65536;
    static constexpr std::string_view kServerName = "libp2p";
    static constexpr std::string_view kClientName = "libp2p";

    HttpToWsUpgrader(
        std::shared_ptr<connection::LayerConnection> connection,
        bool is_initiator, LayerAdaptor::LayerConnCallbackFunc cb,
        std::shared_ptr<basic::Scheduler> scheduler,
        std::shared_ptr<crypto::random::RandomGenerator> random_generator,
        std::shared_ptr<const WsConnectionConfig> config);

    void upgrade();

    enum class Error {
      BAD_REQUEST_BAD_METHOD,
      BAD_REQUEST_BAD_UPDATE_HEADER,
      BAD_REQUEST_BAD_CONNECTION_HEADER,
      BAD_RESPONSE_BAD_STATUS,
      BAD_RESPONSE_BAD_UPDATE_HEADER,
      BAD_RESPONSE_BAD_CONNECTION_HEADER,
      BAD_RESPONSE_BAD_WS_ACCEPT_HEADER,
    };

   private:
    // Outbound connection

    gsl::span<const uint8_t> createHttpRequest();

    void sendHttpUpgradeRequest(gsl::span<const uint8_t> payload,
                                basic::Writer::WriteCallbackFunc cb);

    void readHttpUpgradeResponse(basic::MessageReadWriter::ReadCallbackFunc cb);

    outcome::result<void> handleRemoteResponse(
        gsl::span<const uint8_t> payload);

    // Inbound connection

    void readHttpUpgradeRequest(basic::MessageReadWriter::ReadCallbackFunc cb);

    outcome::result<void> handleRemoteRequest(gsl::span<const uint8_t> payload);

    gsl::span<const uint8_t> createHttpResponse();

    void sendHttpUpgradeResponse(gsl::span<const uint8_t> payload,
                                 basic::Writer::WriteCallbackFunc cb);

    // launch and result

    outcome::result<void> run();
    void onUpgraded(outcome::result<void> result);

    //

    std::shared_ptr<connection::LayerConnection> conn_;
    bool initiator_;
    LayerAdaptor::LayerConnCallbackFunc connection_cb_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<crypto::random::RandomGenerator> random_generator_;
    std::shared_ptr<const WsConnectionConfig> config_;

    std::shared_ptr<common::ByteArray> read_buffer_;
    std::shared_ptr<HttpReadWriter> rw_;

    std::string request_;
    std::string response_;

    std::optional<std::string> key_;

    log::Logger log_ = log::createLogger("HttpToWsUpgrader");
  };

}  // namespace libp2p::layer::websocket

OUTCOME_HPP_DECLARE_ERROR(libp2p::layer::websocket, HttpToWsUpgrader::Error);

#endif  // LIBP2P_LAYER_WEBSOCKET_HTTPTOWSUPGRADER
