/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/ssl/stream.hpp>

#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/as_asio_read_write.hpp>
#include <libp2p/connection/layer_connection.hpp>

namespace libp2p::layer {
  class WssAdaptor;
}  // namespace libp2p::layer

namespace libp2p::connection {
  class SslConnection final
      : public LayerConnection,
        public std::enable_shared_from_this<SslConnection> {
   public:
    SslConnection(std::shared_ptr<boost::asio::io_context> io_context,
                  std::shared_ptr<LayerConnection> connection,
                  std::shared_ptr<boost::asio::ssl::context> ssl_context);

    bool isInitiator() const noexcept override;
    outcome::result<multi::Multiaddress> localMultiaddr() override;
    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    outcome::result<void> close() override;
    bool isClosed() const override;

    void read(BytesOut out, size_t bytes, ReadCallbackFunc cb) override;
    void readSome(BytesOut out, size_t bytes, ReadCallbackFunc cb) override;
    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    void write(BytesIn in, size_t bytes, WriteCallbackFunc cb) override;
    void writeSome(BytesIn in, size_t bytes, WriteCallbackFunc cb) override;
    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

   private:
    std::shared_ptr<LayerConnection> connection_;
    std::shared_ptr<boost::asio::ssl::context> ssl_context_;
    boost::asio::ssl::stream<AsAsioReadWrite> ssl_;

    friend class layer::WssAdaptor;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(libp2p::connection::SslConnection);
  };
}  // namespace libp2p::connection
