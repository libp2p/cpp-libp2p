/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/noncopyable.hpp>

#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/as_asio_read_write.hpp>
#include <libp2p/connection/secure_connection.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/security/tls/tls_errors.hpp>

namespace libp2p::connection {

  /// Secure connection of TLS 1.3 protocol
  class TlsConnection : public SecureConnection,
                        public std::enable_shared_from_this<TlsConnection>,
                        private boost::noncopyable {
   public:
    using ssl_socket_t = boost::asio::ssl::stream<AsAsioReadWrite>;

    /// Upgraded connection passed to this callback
    using HandshakeCallback = std::function<void(
        outcome::result<std::shared_ptr<connection::SecureConnection>>)>;

    /// Ctor.
    /// \param original_connection connection, established at the moment
    /// \param ssl_context Wrapper around SSL_CTX
    /// \param idmgr Identity manager, contains this host's keys
    /// \param io_context Asio io context
    /// \param remote_peer Expected peer id of remote peer, has value for
    /// outbound connections
    TlsConnection(std::shared_ptr<LayerConnection> original_connection,
                  std::shared_ptr<boost::asio::ssl::context> ssl_context,
                  const peer::IdentityManager &idmgr,
                  std::shared_ptr<boost::asio::io_context> io_context,
                  boost::optional<peer::PeerId> remote_peer);

    /// Performs async handshake and passes its result into callback. This fn is
    /// distinct from the ctor because it uses shared_from_this()
    /// \param cb Protocol upgraders callback
    /// \param key_marshaller Key marshaller, we need it to deal with our tricky
    /// certificate extension
    void asyncHandshake(
        HandshakeCallback cb,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller);

    /// Dtor.
    /// TODO(artem): research whether the connection is closed automatically
    ~TlsConnection() override = default;

    /// Returns local peer id
    outcome::result<peer::PeerId> localPeer() const override;

    /// Returns remote peer id, must exist after successful handshake
    outcome::result<peer::PeerId> remotePeer() const override;

    /// Returns remote public key, must exist after successful handshake
    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    /// Returns true if connection is outbound
    bool isInitiator() const override;

    /// Returns local network address
    outcome::result<multi::Multiaddress> localMultiaddr() override;

    /// Returns remote network address
    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    /// Async reads up to the # of bytes given
    void readSome(BytesOut out, ReadCallbackFunc cb) override;

    /// Defers read callback to avoid reentrancy in async calls
    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    /// Async writes up to the # of bytes given
    void writeSome(BytesIn in, WriteCallbackFunc cb) override;

    /// Defers error callback to avoid reentrancy in async calls
    void deferWriteCallback(std::error_code ec, ReadCallbackFunc cb) override;

    /// Returns true if raw connection is closed
    bool isClosed() const override;

    /// Closes the socket
    outcome::result<void> close() override;

   private:
    /// Async handshake callback. Performs libp2p-specific verification and
    /// extraction of remote peer's identity fields
    void onHandshakeResult(
        const boost::system::error_code &error,
        const HandshakeCallback &cb,
        const crypto::marshaller::KeyMarshaller &key_marshaller);

    /// Local peer id
    const peer::PeerId local_peer_;

    /// Raw connection
    std::shared_ptr<LayerConnection> original_connection_;

    /// SSL context, shared among connections
    std::shared_ptr<boost::asio::ssl::context> ssl_context_;

    /// SSL stream
    ssl_socket_t socket_;

    /// Remote peer id
    boost::optional<peer::PeerId> remote_peer_;

    /// Remote public key, extracted from peer certificate during handshake
    boost::optional<crypto::PublicKey> remote_pubkey_;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(libp2p::connection::TlsConnection);
  };
}  // namespace libp2p::connection
