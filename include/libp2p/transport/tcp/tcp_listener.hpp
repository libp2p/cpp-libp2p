/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TCP_LISTENER_HPP
#define LIBP2P_TCP_LISTENER_HPP

#include <boost/asio.hpp>
#include <libp2p/transport/tcp/tcp_connection.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>
#include <libp2p/transport/transport_listener.hpp>
#include <libp2p/transport/upgrader.hpp>

namespace libp2p::transport {

  /**
   * @brief TCP Server (Listener) implementation.
   */
  class TcpListener : public TransportListener,
                      public std::enable_shared_from_this<TcpListener> {
   public:
    using ProtoAddrVec = std::vector<std::pair<multi::Protocol, std::string>>;

    ~TcpListener() override = default;

    TcpListener(boost::asio::io_context &context,
                std::shared_ptr<Upgrader> upgrader,
                TransportListener::HandlerFunc handler);

    outcome::result<void> listen(const multi::Multiaddress &address) override;

    bool canListen(const multi::Multiaddress &ma) const override;

    outcome::result<multi::Multiaddress> getListenMultiaddr() const override;

    bool isClosed() const override;

    outcome::result<void> close() override;

   private:
    boost::asio::io_context &context_;
    std::shared_ptr<Upgrader> upgrader_;
    TransportListener::HandlerFunc handle_;

    boost::asio::ip::tcp::acceptor acceptor_;

    ProtoAddrVec layers_;

    void doAccept();
  };

}  // namespace libp2p::transport

#endif  // LIBP2P_TCP_LISTENER_HPP
