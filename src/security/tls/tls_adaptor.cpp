/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/tls/tls_adaptor.hpp>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/security/tls/tls_errors.hpp>
#include <libp2p/transport/tcp/tcp_connection.hpp>

#include "tls_connection.hpp"
#include "tls_details.hpp"

namespace libp2p::security {

  using connection::TlsConnection;
  using tls_details::log;

  TlsAdaptor::TlsAdaptor(
      std::shared_ptr<peer::IdentityManager> idmgr,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : idmgr_(std::move(idmgr)),
        io_context_(std::move(io_context)),
        key_marshaller_(std::move(key_marshaller)) {
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
      const peer::PeerId &p, SecConnCallbackFunc cb) {
    asyncHandshake(std::move(outbound), p, std::move(cb));
  }

  outcome::result<void> TlsAdaptor::setupContext() {
    assert(!ssl_context_);
    using cbase = boost::asio::ssl::context_base;

    try {
      ssl_context_ = std::make_shared<boost::asio::ssl::context>(cbase::tlsv13);
      ssl_context_->set_options(cbase::no_compression | cbase::no_sslv2
                                | cbase::no_sslv3 | cbase::no_tlsv1_1
                                | cbase::no_tlsv1_2);
      ssl_context_->set_verify_mode(
          boost::asio::ssl::verify_peer
          | boost::asio::ssl::verify_fail_if_no_peer_cert
          | boost::asio::ssl::verify_client_once);
      ssl_context_->set_verify_callback(&tls_details::verifyCallback);

      auto ck =
          tls_details::makeCertificate(idmgr_->getKeyPair(), *key_marshaller_);

      ssl_context_->use_certificate(
          boost::asio::const_buffer(ck.certificate.data(),
                                    ck.certificate.size()),
          boost::asio::ssl::context_base::asn1);

      ssl_context_->use_private_key(
          boost::asio::const_buffer(ck.private_key.data(),
                                    ck.private_key.size()),
          boost::asio::ssl::context_base::asn1);
    } catch (const std::exception &e) {
      ssl_context_.reset();
      log()->error("context init failed: {}", e.what());
      return TlsError::TLS_CTX_INIT_FAILED;
    }

    return outcome::success();
  }

  void TlsAdaptor::asyncHandshake(
      std::shared_ptr<connection::LayerConnection> conn,
      boost::optional<peer::PeerId> remote_peer, SecConnCallbackFunc cb) {
    bool is_client = conn->isInitiator();

    if (is_client) {
      SL_DEBUG(log(), "securing outbound connection");
      assert(remote_peer.has_value());
    } else {
      SL_DEBUG(log(), "securing inbound connection");
    }

    std::error_code ec;
    if (!ssl_context_) {
      auto res = setupContext();
      if (!res) {
        ec = res.error();
      }
    }

    transport::TcpConnection *tcp_conn = nullptr;

    if (!ec) {
      tcp_conn = dynamic_cast<transport::TcpConnection *>(conn.get());
      if (tcp_conn == nullptr) {
        ec = TlsError::TLS_INCOMPATIBLE_TRANSPORT;
      } else {
        auto tls_conn = std::make_shared<TlsConnection>(
            std::move(conn), ssl_context_, *idmgr_, tcp_conn->socket_,
            std::move(remote_peer));
        tls_conn->asyncHandshake(std::move(cb), key_marshaller_);
      }
    }

    if (ec) {
      io_context_->post([cb, ec] { cb(ec); });  // NOLINT
    }
  }

}  // namespace libp2p::security
