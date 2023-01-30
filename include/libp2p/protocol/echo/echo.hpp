/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_ECHO_IMPL_HPP
#define LIBP2P_ECHO_IMPL_HPP

#include <libp2p/log/logger.hpp>
#include <libp2p/peer/protocol.hpp>
#include <libp2p/protocol/base_protocol.hpp>
#include <libp2p/protocol/echo/client_echo_session.hpp>
#include <libp2p/protocol/echo/echo_config.hpp>
#include <libp2p/protocol/echo/server_echo_session.hpp>

namespace libp2p::protocol {

  /**
   * @brief Simple echo protocol. It will keep responding with the same data it
   * reads from the connection.
   */
  class Echo : public BaseProtocol {
   public:
    explicit Echo(EchoConfig config = EchoConfig{});

    // NOLINTNEXTLINE(modernize-use-nodiscard)
    peer::ProtocolName getProtocolId() const override;

    // handle incoming stream
    void handle(StreamAndProtocol stream) override;

    // create client session, which simplifies writing tests and interaction
    // with server.
    std::shared_ptr<ClientEchoSession> createClient(
        const std::shared_ptr<connection::Stream> &stream);

   private:
    EchoConfig config_;
    log::Logger log_ = log::createLogger("Echo");
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_ECHO_IMPL_HPP
