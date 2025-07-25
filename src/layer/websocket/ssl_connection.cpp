/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ssl_connection.hpp>

#include <libp2p/common/asio_buffer.hpp>
#include <libp2p/common/asio_cb.hpp>

namespace libp2p::connection {
  SslConnection::SslConnection(
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<LayerConnection> connection,
      std::shared_ptr<boost::asio::ssl::context> ssl_context)
      : connection_{std::move(connection)},
        ssl_context_{std::move(ssl_context)},
        ssl_{
            AsAsioReadWrite{std::move(io_context), connection_},
            *ssl_context_,
        } {}

  bool SslConnection::isInitiator() const {
    return connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> SslConnection::localMultiaddr() {
    return connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> SslConnection::remoteMultiaddr() {
    return connection_->remoteMultiaddr();
  }

  bool SslConnection::isClosed() const {
    return connection_->isClosed();
  }

  outcome::result<void> SslConnection::close() {
    return connection_->close();
  }

  void SslConnection::readSome(BytesOut out,
                               libp2p::basic::Reader::ReadCallbackFunc cb) {
    ssl_.async_read_some(asioBuffer(out), toAsioCbSize(std::move(cb)));
  }

  void SslConnection::writeSome(BytesIn in,
                                libp2p::basic::Writer::WriteCallbackFunc cb) {
    ssl_.async_write_some(asioBuffer(in), toAsioCbSize(std::move(cb)));
  }

  void SslConnection::deferReadCallback(outcome::result<size_t> res,
                                        ReadCallbackFunc cb) {
    connection_->deferReadCallback(res, std::move(cb));
  }

  void SslConnection::deferWriteCallback(std::error_code ec,
                                         WriteCallbackFunc cb) {
    connection_->deferWriteCallback(ec, std::move(cb));
  }
}  // namespace libp2p::connection
