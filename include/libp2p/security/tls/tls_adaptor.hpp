/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECURITY_TLS_ADAPTOR_HPP
#define LIBP2P_SECURITY_TLS_ADAPTOR_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>

#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/security/security_adaptor.hpp>
#include <libp2p/security/tls/tls_errors.hpp>

namespace libp2p::security {

  /// TLS 1.3 security adaptor
  class TlsAdaptor : public SecurityAdaptor,
                     public std::enable_shared_from_this<TlsAdaptor> {
   public:
    /// Dtor.
    ~TlsAdaptor() override = default;

    /// Ctor.
    TlsAdaptor(
        std::shared_ptr<peer::IdentityManager> idmgr,
        std::shared_ptr<boost::asio::io_context> io_context,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller);

    /// Returns "/tls/1.0.0"
    peer::Protocol getProtocolId() const override;

    /// Performs async handshake for inbound connection
    void secureInbound(std::shared_ptr<connection::RawConnection> inbound,
                       SecConnCallbackFunc cb) override;

    /// Performs async handshake for outbound connection
    void secureOutbound(std::shared_ptr<connection::RawConnection> outbound,
                        const peer::PeerId &p, SecConnCallbackFunc cb) override;

   private:
    /// Creates shared SSL context, generates certificate and private key
    outcome::result<void> setupContext();

    /// Creates TLSConnection and starts handshake
    void asyncHandshake(std::shared_ptr<connection::RawConnection> conn,
                         boost::optional<peer::PeerId> remote_peer,
                         SecConnCallbackFunc cb);

    /// Identity manager which contains this host's keys and peer id
    std::shared_ptr<peer::IdentityManager> idmgr_;

    /// IO context, used to defer callback on error
    std::shared_ptr<boost::asio::io_context> io_context_;

    /// Key marshaller, needed for custom cert extension
    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;

    /// Shared ssl context
    std::shared_ptr<boost::asio::ssl::context> ssl_context_;
  };
}  // namespace libp2p::security

#endif  // LIBP2P_SECURITY_TLS_ADAPTOR_HPP
