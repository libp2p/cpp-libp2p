/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/transport/transport_listener.hpp>

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace boost::asio::ssl {
  class context;
}  // namespace boost::asio::ssl

namespace libp2p::crypto::marshaller {
  class KeyMarshaller;
}  // namespace libp2p::crypto::marshaller

namespace libp2p::transport::lsquic {
  class Engine;
}  // namespace libp2p::transport::lsquic

namespace libp2p::transport {
  class QuicListener : public TransportListener,
                       public std::enable_shared_from_this<QuicListener> {
   public:
    QuicListener(std::shared_ptr<boost::asio::io_context> io_context,
                 std::shared_ptr<boost::asio::ssl::context> ssl_context,
                 const muxer::MuxedConnectionConfig &mux_config,
                 PeerId local_peer,
                 std::shared_ptr<crypto::marshaller::KeyMarshaller> key_codec,
                 TransportListener::HandlerFunc handler);

    // Closeable
    bool isClosed() const override;
    outcome::result<void> close() override;

    // TransportListener
    outcome::result<void> listen(const Multiaddress &address) override;
    bool canListen(const Multiaddress &ma) const override;
    outcome::result<Multiaddress> getListenMultiaddr() const override;

   private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<boost::asio::ssl::context> ssl_context_;
    muxer::MuxedConnectionConfig mux_config_;
    PeerId local_peer_;
    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_codec_;
    TransportListener::HandlerFunc handler_;
    std::shared_ptr<lsquic::Engine> server_;
  };
}  // namespace libp2p::transport
