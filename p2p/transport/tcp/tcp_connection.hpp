/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TCP_CONNECTION_HPP
#define LIBP2P_TCP_CONNECTION_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include "connection/raw_connection.hpp"
#include "multi/multiaddress.hpp"

namespace libp2p::transport {

  /**
   * @brief boost::asio implementation of TCP connection (socket).
   */
  class TcpConnection : public connection::RawConnection,
                        public std::enable_shared_from_this<TcpConnection>,
                        private boost::noncopyable {
   public:
    ~TcpConnection() override = default;

    using Tcp = boost::asio::ip::tcp;
    using ErrorCode = boost::system::error_code;
    using ResolverResultsType = Tcp::resolver::results_type;
    using ResolveCallback = void(const ErrorCode &,
                                 const ResolverResultsType &);
    using ResolveCallbackFunc = std::function<ResolveCallback>;
    using ConnectCallback = void(const ErrorCode &, const Tcp::endpoint &);
    using ConnectCallbackFunc = std::function<ConnectCallback>;

    explicit TcpConnection(boost::asio::io_context &ctx);

    TcpConnection(boost::asio::io_context &ctx, Tcp::socket &&socket);

    /**
     * @brief Resolve service name (DNS).
     * @param endpoint endpoint to resolve.
     * @param cb callback executed on operation completion.
     */
    void resolve(const Tcp::endpoint &endpoint, ResolveCallbackFunc cb);

    /**
     * @brief Connect to a remote service.
     * @param iterator list of resolved IP addresses of remote service.
     * @param cb callback executed on operation completion.
     */
    void connect(const ResolverResultsType &iterator, ConnectCallbackFunc cb);

    void read(gsl::span<uint8_t> out,
              size_t bytes,
              ReadCallbackFunc cb) override;

    void readSome(gsl::span<uint8_t> out,
                  size_t bytes,
                  ReadCallbackFunc cb) override;

    void write(gsl::span<const uint8_t> in,
               size_t bytes,
               WriteCallbackFunc cb) override;

    void writeSome(gsl::span<const uint8_t> in,
                   size_t bytes,
                   WriteCallbackFunc cb) override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    bool isInitiator() const noexcept override;

    outcome::result<void> close() override;

    bool isClosed() const override;

   private:
    boost::asio::io_context &context_;
    Tcp::socket socket_;
    bool initiator_ = false;

    boost::system::error_code handle_errcode(
        const boost::system::error_code &e) noexcept;
  };
}  // namespace libp2p::transport

#endif  // LIBP2P_TCP_CONNECTION_HPP
