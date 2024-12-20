/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/transport/tcp/tcp_listener.hpp>
#include <libp2p/transport/transport_adaptor.hpp>
#include <libp2p/transport/upgrader.hpp>

namespace libp2p::transport {

  /**
   * @brief TCP Transport implementation
   */
  class TcpTransport : public TransportAdaptor,
                       public std::enable_shared_from_this<TcpTransport> {
   public:
    ~TcpTransport() override = default;

    TcpTransport(std::shared_ptr<boost::asio::io_context> context,
                 const muxer::MuxedConnectionConfig &mux_config,
                 std::shared_ptr<Upgrader> upgrader);

    void dial(const peer::PeerId &remoteId,
              multi::Multiaddress address,
              TransportAdaptor::HandlerFunc handler) override;

    std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) override;

    bool canDial(const multi::Multiaddress &ma) const override;

    peer::ProtocolName getProtocolId() const override;

   private:
    std::shared_ptr<boost::asio::io_context> context_;
    muxer::MuxedConnectionConfig mux_config_;
    std::shared_ptr<Upgrader> upgrader_;
    boost::asio::ip::tcp::resolver resolver_;
  };
}  // namespace libp2p::transport
