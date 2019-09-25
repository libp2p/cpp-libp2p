/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ECHO_IMPL_HPP
#define KAGOME_ECHO_IMPL_HPP

//#include "common/logger.hpp"
#include "peer/protocol.hpp"
#include "protocol/base_protocol.hpp"
#include "protocol/echo/client_echo_session.hpp"
#include "protocol/echo/echo_config.hpp"
#include "protocol/echo/server_echo_session.hpp"

namespace libp2p::protocol {

  /**
   * @brief Simple echo protocol. It will keep responding with the same data it
   * reads from the connection.
   */
  class Echo : public BaseProtocol {
   public:
    explicit Echo(EchoConfig config = EchoConfig{});

    // NOLINTNEXTLINE(modernize-use-nodiscard)
    peer::Protocol getProtocolId() const override;

    // handle incoming stream
    void handle(StreamResult rstream) override;

    // create client session, which simplifies writing tests and interaction
    // with server.
    std::shared_ptr<ClientEchoSession> createClient(
        const std::shared_ptr<connection::Stream> &stream);

   private:
    EchoConfig config_;
//    kagome::common::Logger log_ = kagome::common::createLogger("echo");
  };

}  // namespace libp2p::protocol

#endif  // KAGOME_ECHO_IMPL_HPP
