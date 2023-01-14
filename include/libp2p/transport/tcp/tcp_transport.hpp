/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TCP_TRANSPORT_HPP
#define LIBP2P_TCP_TRANSPORT_HPP

#include <boost/asio.hpp>
#include <libp2p/transport/tcp/tcp_listener.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>
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
                 std::shared_ptr<Upgrader> upgrader);

    void dial(const peer::PeerId &remoteId, multi::Multiaddress address,
              TransportAdaptor::HandlerFunc handler) override;

    void dial(const peer::PeerId &remoteId, multi::Multiaddress address,
              TransportAdaptor::HandlerFunc handler,
              std::chrono::milliseconds timeout) override;

    std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) override;

    bool canDial(const multi::Multiaddress &ma) const override;

    peer::ProtocolName getProtocolId() const override;

   private:
    std::shared_ptr<boost::asio::io_context> context_;
    std::shared_ptr<Upgrader> upgrader_;
  };  // namespace libp2p::transport

}  // namespace libp2p::transport

#endif  // LIBP2P_TCP_TRANSPORT_HPP
