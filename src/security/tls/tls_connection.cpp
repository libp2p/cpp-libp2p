/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/asio_buffer.hpp>
#include <libp2p/security/tls/tls_connection.hpp>
#include <libp2p/security/tls/tls_details.hpp>

namespace libp2p::connection {

  using TlsError = security::TlsError;
  using security::tls_details::log;

  TlsConnection::TlsConnection(
      std::shared_ptr<LayerConnection> original_connection,
      std::shared_ptr<boost::asio::ssl::context> ssl_context,
      const peer::IdentityManager &idmgr,
      std::shared_ptr<boost::asio::io_context> io_context,
      boost::optional<peer::PeerId> remote_peer)
      : local_peer_(idmgr.getId()),
        original_connection_(std::move(original_connection)),
        ssl_context_(std::move(ssl_context)),
        socket_{AsAsioReadWrite{std::move(io_context), original_connection_},
                *ssl_context_},
        remote_peer_(std::move(remote_peer)) {}

  void TlsConnection::asyncHandshake(
      HandshakeCallback cb,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller) {
    bool is_client = original_connection_->isInitiator();

    socket_.async_handshake(is_client ? boost::asio::ssl::stream_base::client
                                      : boost::asio::ssl::stream_base::server,
                            [self = shared_from_this(),
                             cb = std::move(cb),
                             key_marshaller = std::move(key_marshaller)](
                                const boost::system::error_code &error) {
                              self->onHandshakeResult(
                                  error, cb, *key_marshaller);
                            });
  }

  void TlsConnection::onHandshakeResult(
      const boost::system::error_code &error,
      const HandshakeCallback &cb,
      const crypto::marshaller::KeyMarshaller &key_marshaller) {
    std::optional<std::error_code> ec;
    if (error) {
      ec = error;
    }
    while (!ec) {
      X509 *cert = SSL_get_peer_certificate(socket_.native_handle());
      if (cert == nullptr) {
        ec = TlsError::TLS_NO_CERTIFICATE;
        break;
      }
      auto id_res = security::tls_details::verifyPeerAndExtractIdentity(
          cert, key_marshaller);
      if (!id_res) {
        ec = id_res.error();
        break;
      }
      auto &id = id_res.value();
      if (remote_peer_.has_value()) {
        if (remote_peer_.value() != id.peer_id) {
          SL_DEBUG(log(),
                   "peer ids mismatch: expected={}, got={}",
                   remote_peer_.value().toBase58(),
                   id.peer_id.toBase58());
          ec = TlsError::TLS_UNEXPECTED_PEER_ID;
          break;
        }
      } else {
        remote_peer_ = std::move(id.peer_id);
      }
      remote_pubkey_ = std::move(id.public_key);

      SL_DEBUG(log(),
               "handshake success for {}bound connection to {}",
               (original_connection_->isInitiator() ? "out" : "in"),
               remote_peer_->toBase58());
      return cb(shared_from_this());
    }

    assert(ec);

    log()->info("handshake error: {}", *ec);
    if (auto close_res = close(); !close_res) {
      log()->info("cannot close raw connection: {}", close_res.error());
    }
    return cb(*ec);
  }

  outcome::result<peer::PeerId> TlsConnection::localPeer() const {
    return local_peer_;
  }

  outcome::result<peer::PeerId> TlsConnection::remotePeer() const {
    if (!remote_peer_) {
      return TlsError::TLS_REMOTE_PEER_NOT_AVAILABLE;
    }
    return remote_peer_.value();
  }

  outcome::result<crypto::PublicKey> TlsConnection::remotePublicKey() const {
    if (!remote_pubkey_) {
      return TlsError::TLS_REMOTE_PUBKEY_NOT_AVAILABLE;
    }
    return remote_pubkey_.value();
  }

  bool TlsConnection::isInitiator() const {
    return original_connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> TlsConnection::localMultiaddr() {
    return original_connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> TlsConnection::remoteMultiaddr() {
    return original_connection_->remoteMultiaddr();
  }

  template <typename Callback>
  auto closeOnError(TlsConnection &conn, Callback cb) {
    return [cb{std::move(cb)}, conn{conn.shared_from_this()}](auto &&ec,
                                                              auto &&result) {
      if (ec) {
        SL_DEBUG(log(), "connection async op error {}", ec);
        cb(ec);
        std::ignore = conn->close();
        return;
      }
      cb(std::forward<decltype(result)>(result));
    };
  }

  void TlsConnection::readSome(BytesOut out, Reader::ReadCallbackFunc cb) {
    SL_TRACE(log(), "reading some up to {} bytes", out.size());
    socket_.async_read_some(asioBuffer(out),
                            closeOnError(*this, std::move(cb)));
  }

  void TlsConnection::deferReadCallback(outcome::result<size_t> res,
                                        Reader::ReadCallbackFunc cb) {
    original_connection_->deferReadCallback(res, std::move(cb));
  }

  void TlsConnection::writeSome(BytesIn in, Writer::WriteCallbackFunc cb) {
    SL_TRACE(log(), "writing some up to {} bytes", in.size());
    socket_.async_write_some(asioBuffer(in),
                             closeOnError(*this, std::move(cb)));
  }

  void TlsConnection::deferWriteCallback(std::error_code ec,
                                         Writer::WriteCallbackFunc cb) {
    original_connection_->deferWriteCallback(ec, std::move(cb));
  }

  bool TlsConnection::isClosed() const {
    return original_connection_->isClosed();
  }

  outcome::result<void> TlsConnection::close() {
    return original_connection_->close();
  }
}  // namespace libp2p::connection
