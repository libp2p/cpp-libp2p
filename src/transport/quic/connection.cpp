/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/quic/connection.hpp>
#include <libp2p/transport/quic/engine.hpp>
#include <libp2p/transport/quic/error.hpp>
#include <libp2p/transport/quic/stream.hpp>

namespace libp2p::transport {
  QuicConnection::QuicConnection(
      std::shared_ptr<boost::asio::io_context> io_context,
      lsquic::ConnCtx *conn_ctx,
      bool initiator,
      Multiaddress local,
      Multiaddress remote,
      PeerId local_peer,
      PeerId peer,
      crypto::PublicKey key)
      : io_context_{std::move(io_context)},
        conn_ctx_{conn_ctx},
        initiator_{initiator},
        local_{std::move(local)},
        remote_{std::move(remote)},
        local_peer_{std::move(local_peer)},
        peer_{std::move(peer)},
        key_{std::move(key)} {}

  QuicConnection::~QuicConnection() {
    std::ignore = close();
  }

  void QuicConnection::read(BytesOut out, size_t bytes, ReadCallbackFunc cb) {
    throw std::logic_error{"QuicConnection::read must not be called"};
  }

  void QuicConnection::readSome(BytesOut out,
                                size_t bytes,
                                ReadCallbackFunc cb) {
    throw std::logic_error{"QuicConnection::readSome must not be called"};
  }

  void QuicConnection::deferReadCallback(outcome::result<size_t> res,
                                         ReadCallbackFunc cb) {
    io_context_->post([cb{std::move(cb)}, res] { cb(res); });
  }

  void QuicConnection::writeSome(BytesIn in,
                                 size_t bytes,
                                 WriteCallbackFunc cb) {
    throw std::logic_error{"QuicConnection::writeSome must not be called"};
  }

  void QuicConnection::deferWriteCallback(std::error_code ec,
                                          WriteCallbackFunc cb) {
    deferReadCallback(ec, std::move(cb));
  }

  bool QuicConnection::isClosed() const {
    return not conn_ctx_;
  }

  outcome::result<void> QuicConnection::close() {
    if (conn_ctx_) {
      lsquic_conn_close(conn_ctx_->ls_conn);
    }
    return outcome::success();
  }

  bool QuicConnection::isInitiator() const noexcept {
    return initiator_;
  }

  outcome::result<Multiaddress> QuicConnection::remoteMultiaddr() {
    return remote_;
  }

  outcome::result<Multiaddress> QuicConnection::localMultiaddr() {
    return local_;
  }

  outcome::result<PeerId> QuicConnection::localPeer() const {
    return local_peer_;
  }

  outcome::result<PeerId> QuicConnection::remotePeer() const {
    return peer_;
  }

  outcome::result<crypto::PublicKey> QuicConnection::remotePublicKey() const {
    return key_;
  }

  void QuicConnection::start() {}

  void QuicConnection::stop() {}

  void QuicConnection::newStream(CapableConnection::StreamHandlerFunc cb) {
    cb(newStream());
  }

  outcome::result<std::shared_ptr<libp2p::connection::Stream>>
  QuicConnection::newStream() {
    if (not conn_ctx_) {
      return QuicError::CONN_CLOSED;
    }
    OUTCOME_TRY(stream, conn_ctx_->engine->newStream(conn_ctx_));
    return stream;
  }

  void QuicConnection::onStream(NewStreamHandlerFunc cb) {
    on_stream_ = std::move(cb);
  }

  void QuicConnection::onClose() {
    conn_ctx_ = nullptr;
  }
}  // namespace libp2p::transport
