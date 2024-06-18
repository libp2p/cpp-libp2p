/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/tls/tls_adaptor.hpp>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/security/tls/ssl_context.hpp>
#include <libp2p/security/tls/tls_details.hpp>
#include <libp2p/security/tls/tls_errors.hpp>
#include <libp2p/transport/tcp/tcp_connection.hpp>

#include "tls_connection.hpp"

namespace libp2p::security {

  using connection::TlsConnection;
  using tls_details::log;

  TlsAdaptor::TlsAdaptor(
      std::shared_ptr<peer::IdentityManager> idmgr,
      std::shared_ptr<boost::asio::io_context> io_context,
      const SslContext &ssl_context,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : idmgr_(std::move(idmgr)),
        io_context_(std::move(io_context)),
        key_marshaller_{std::move(key_marshaller)},
        ssl_context_{ssl_context.tls} {
    assert(idmgr_);
    assert(io_context_);
    assert(key_marshaller_);
  }

  peer::ProtocolName TlsAdaptor::getProtocolId() const {
    return "/tls/1.0.0";
  }

  void TlsAdaptor::secureInbound(
      std::shared_ptr<connection::LayerConnection> inbound,
      SecConnCallbackFunc cb) {
    asyncHandshake(std::move(inbound), boost::none, std::move(cb));
  }

  void TlsAdaptor::secureOutbound(
      std::shared_ptr<connection::LayerConnection> outbound,
      const peer::PeerId &p,
      SecConnCallbackFunc cb) {
    asyncHandshake(std::move(outbound), p, std::move(cb));
  }

  void TlsAdaptor::asyncHandshake(
      std::shared_ptr<connection::LayerConnection> conn,
      boost::optional<peer::PeerId> remote_peer,
      SecConnCallbackFunc cb) {
    bool is_client = conn->isInitiator();

    if (is_client) {
      SL_DEBUG(log(), "securing outbound connection");
      assert(remote_peer.has_value());
    } else {
      SL_DEBUG(log(), "securing inbound connection");
    }

    std::optional<std::error_code> ec;

    transport::TcpConnection *tcp_conn = nullptr;

    if (!ec) {
      tcp_conn = dynamic_cast<transport::TcpConnection *>(conn.get());
      if (tcp_conn == nullptr) {
        ec = TlsError::TLS_INCOMPATIBLE_TRANSPORT;
      } else {
        auto tls_conn = std::make_shared<TlsConnection>(std::move(conn),
                                                        ssl_context_,
                                                        *idmgr_,
                                                        tcp_conn->socket_,
                                                        std::move(remote_peer));
        tls_conn->asyncHandshake(std::move(cb), key_marshaller_);
      }
    }

    if (ec) {
      io_context_->post([cb, ec] { cb(*ec); });
    }
  }

}  // namespace libp2p::security
