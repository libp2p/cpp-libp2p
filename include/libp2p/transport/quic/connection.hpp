/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/capable_connection.hpp>

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace libp2p::transport::lsquic {
  class Engine;
  struct ConnCtx;
}  // namespace libp2p::transport::lsquic

namespace libp2p::transport {
  class QuicConnection : public connection::CapableConnection,
                         public std::enable_shared_from_this<QuicConnection> {
   public:
    QuicConnection(std::shared_ptr<boost::asio::io_context> io_context,
                   lsquic::ConnCtx *conn_ctx,
                   bool initiator,
                   Multiaddress local,
                   Multiaddress remote,
                   PeerId local_peer,
                   PeerId peer,
                   crypto::PublicKey key);
    ~QuicConnection() override;

    // clang-tidy cppcoreguidelines-special-member-functions
    QuicConnection(const QuicConnection &) = delete;
    void operator=(const QuicConnection &) = delete;
    QuicConnection(QuicConnection &&) = delete;
    void operator=(QuicConnection &&) = delete;

    // Reader
    void read(BytesOut out, size_t bytes, ReadCallbackFunc cb) override;
    void readSome(BytesOut out, size_t bytes, ReadCallbackFunc cb) override;
    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    // Writer
    void writeSome(BytesIn in, size_t bytes, WriteCallbackFunc cb) override;
    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

    // Closeable
    bool isClosed() const override;
    outcome::result<void> close() override;

    // LayerConnection
    bool isInitiator() const noexcept override;
    outcome::result<Multiaddress> remoteMultiaddr() override;
    outcome::result<Multiaddress> localMultiaddr() override;

    // SecureConnection
    outcome::result<PeerId> localPeer() const override;
    outcome::result<PeerId> remotePeer() const override;
    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    // CapableConnection
    void start() override;
    void stop() override;
    void newStream(StreamHandlerFunc cb) override;
    outcome::result<std::shared_ptr<libp2p::connection::Stream>> newStream()
        override;
    void onStream(NewStreamHandlerFunc cb) override;

    void onClose();
    auto &onStream() const {
      return on_stream_;
    }

   private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    lsquic::ConnCtx *conn_ctx_;
    bool initiator_;
    Multiaddress local_, remote_;
    PeerId local_peer_, peer_;
    crypto::PublicKey key_;
    NewStreamHandlerFunc on_stream_;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(libp2p::transport::QuicConnection);
  };
}  // namespace libp2p::transport
