/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/security/tls/ssl_context.hpp>
#include <libp2p/transport/quic/connection.hpp>
#include <libp2p/transport/quic/engine.hpp>
#include <libp2p/transport/quic/listener.hpp>
#include <libp2p/transport/quic/transport.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>

namespace libp2p::transport {
  QuicTransport::QuicTransport(
      std::shared_ptr<boost::asio::io_context> io_context,
      const security::SslContext &ssl_context,
      const muxer::MuxedConnectionConfig &mux_config,
      const peer::IdentityManager &id_mgr,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_codec)
      : io_context_{std::move(io_context)},
        ssl_context_{ssl_context.quic},
        mux_config_{mux_config},
        local_peer_{id_mgr.getId()},
        key_codec_{std::move(key_codec)},
        resolver_{*io_context_},
        client4_{makeClient(boost::asio::ip::udp::v4())},
        client6_{makeClient(boost::asio::ip::udp::v6())} {}

  void QuicTransport::dial(const PeerId &peer,
                           Multiaddress address,
                           TransportAdaptor::HandlerFunc cb,
                           std::chrono::milliseconds timeout) {
    auto r = detail::asQuic(address);
    if (not r) {
      return cb(r.error());
    }
    auto &info = r.value();
    auto connect =
        [weak_self{weak_from_this()}, peer, cb{std::move(cb)}](
            outcome::result<boost::asio::ip::udp::resolver::results_type>
                r) mutable {
          auto self = weak_self.lock();
          if (not self) {
            return;
          }
          if (not r) {
            return cb(r.error());
          }
          auto remote = r.value().begin()->endpoint();
          auto v4 = remote.protocol() == boost::asio::ip::udp::v4();
          auto &client = v4 ? self->client4_ : self->client6_;
          client->connect(
              remote,
              peer,
              [cb{std::move(cb)}](
                  outcome::result<std::shared_ptr<QuicConnection>> r) {
                if (not r) {
                  return cb(r.error());
                }
                cb(r.value());
              });
        };
    detail::resolve(resolver_, info, std::move(connect));
  }

  std::shared_ptr<TransportListener> QuicTransport::createListener(
      TransportListener::HandlerFunc handler) {
    return std::make_shared<QuicListener>(io_context_,
                                          ssl_context_,
                                          mux_config_,
                                          local_peer_,
                                          key_codec_,
                                          std::move(handler));
  }

  bool QuicTransport::canDial(const Multiaddress &ma) const {
    return detail::asQuic(ma).has_value();
  }

  peer::ProtocolName QuicTransport::getProtocolId() const {
    return "/quic/1.0.0";
  }

  std::shared_ptr<lsquic::Engine> QuicTransport::makeClient(
      boost::asio::ip::udp protocol) const {
    return std::make_shared<lsquic::Engine>(io_context_,
                                            ssl_context_,
                                            mux_config_,
                                            local_peer_,
                                            key_codec_,
                                            boost::asio::ip::udp::socket{
                                                *io_context_,
                                                protocol,
                                            },
                                            true);
  }
}  // namespace libp2p::transport
