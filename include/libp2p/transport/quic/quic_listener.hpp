/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio.hpp>
#include <libp2p/transport/quic/libp2p-quic-api.hpp>
#include <libp2p/transport/transport_listener.hpp>
#include <nexus/quic/server.hpp>
#include <libp2p/log/logger.hpp>

namespace boost::asio::ssl
{
  class context;
}

namespace libp2p::transport {
  class Upgrader;

  /**
   * @brief QUIC Server (Listener) implementation.
   */
  class LIBP2P_QUIC_API QuicListener : public TransportListener,
                      public std::enable_shared_from_this<QuicListener> {
   public:
    using ProtoAddrVec = std::vector<std::pair<multi::Protocol, std::string>>;

    ~QuicListener() override = default;

    QuicListener(nexus::quic::server&, boost::asio::ssl::context &,
                TransportListener::HandlerFunc handler); 
    
    // accepted connection is started() before handler is invoked. is this a problem ?!

    outcome::result<void> listen(const multi::Multiaddress &address) override;
    bool canListen(const multi::Multiaddress &ma) const override;
    outcome::result<multi::Multiaddress> getListenMultiaddr() const override;
    // closable
    bool isClosed() const override;
    outcome::result<void> close() override;

   private:
    // boost::asio::io_context &context_;
    //std::shared_ptr<Upgrader> upgrader_; // not needed for quic
    TransportListener::HandlerFunc handle_;

    nexus::quic::server& m_server;
    nexus::quic::acceptor m_acceptor;
    std::optional<multi::Multiaddress> m_listen_addr;

    int m_incoming_conn_capacity{20};
    bool m_is_open=false;
    log::Logger log_;

    void doAcceptConns();
  };

}  // namespace libp2p::transport
