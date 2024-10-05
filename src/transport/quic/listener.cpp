/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/quic/connection.hpp>
#include <libp2p/transport/quic/engine.hpp>
#include <libp2p/transport/quic/listener.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>

namespace libp2p::transport {
  QuicListener::QuicListener(
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<boost::asio::ssl::context> ssl_context,
      const muxer::MuxedConnectionConfig &mux_config,
      PeerId local_peer,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_codec,
      TransportListener::HandlerFunc handler)
      : io_context_{std::move(io_context)},
        ssl_context_{std::move(ssl_context)},
        mux_config_{mux_config},
        local_peer_{std::move(local_peer)},
        key_codec_{std::move(key_codec)},
        handler_{std::move(handler)} {}

  outcome::result<void> QuicListener::listen(const Multiaddress &address) {
    OUTCOME_TRY(info, detail::asQuic(address));
    if (server_) {
      return std::errc::already_connected;
    }
    OUTCOME_TRY(endpoint, info.asUdp());
    boost::asio::ip::udp::socket socket{*io_context_, endpoint.protocol()};
    boost::system::error_code ec;
    socket.bind(endpoint, ec);
    if (ec) {
      return ec;
    }
    server_ = std::make_shared<lsquic::Engine>(io_context_,
                                               ssl_context_,
                                               mux_config_,
                                               local_peer_,
                                               key_codec_,
                                               std::move(socket),
                                               false);
    server_->onAccept(handler_);
    server_->start();
    return outcome::success();
  }

  bool QuicListener::canListen(const Multiaddress &ma) const {
    return detail::asQuic(ma).has_value();
  }

  outcome::result<Multiaddress> QuicListener::getListenMultiaddr() const {
    if (not server_) {
      return std::errc::not_connected;
    }
    return server_->local();
  }

  bool QuicListener::isClosed() const {
    return not server_;
  }

  outcome::result<void> QuicListener::close() {
    server_.reset();
    return outcome::success();
  }
}  // namespace libp2p::transport
