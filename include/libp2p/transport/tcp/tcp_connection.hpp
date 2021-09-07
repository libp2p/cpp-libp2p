/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TCP_CONNECTION_HPP
#define LIBP2P_TCP_CONNECTION_HPP

#include <atomic>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/multi/multiaddress.hpp>

namespace libp2p::security {
  class TlsAdaptor;
}

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
     * @brief Resolve service name (DNS).
     * @param host_name host name to resolve
     * @param cb callback executed on operation completion.
     */
    void resolve(const std::string &host_name, const std::string &port,
                 ResolveCallbackFunc cb);

    /**
     * @brief Resolve service name (DNS).
     * @param protocol is either Tcp::ip4 or Tcp::ip6 protocol
     * @param host_name host name to resolve
     * @param cb callback executed on operation completion.
     */
    void resolve(const Tcp &protocol, const std::string &host_name,
                 const std::string &port, ResolveCallbackFunc cb);

    /**
     * @brief Connect to a remote service.
     * @param iterator list of resolved IP addresses of remote service.
     * @param cb callback executed on operation completion.
     */
    void connect(const ResolverResultsType &iterator, ConnectCallbackFunc cb);

    /**
     * @brief Connect to a remote service with a time limit for connection
     * establishing.
     * @param iterator list of resolved IP addresses of remote service.
     * @param cb callback executed on operation completion.
     * @param timeout in milliseconds for connection establishing.
     */
    void connect(const ResolverResultsType &iterator, ConnectCallbackFunc cb,
                 std::chrono::milliseconds timeout);

    void read(gsl::span<uint8_t> out, size_t bytes,
              ReadCallbackFunc cb) override;

    void readSome(gsl::span<uint8_t> out, size_t bytes,
                  ReadCallbackFunc cb) override;

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    void write(gsl::span<const uint8_t> in, size_t bytes,
               WriteCallbackFunc cb) override;

    void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                   WriteCallbackFunc cb) override;

    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    bool isInitiator() const noexcept override;

    outcome::result<void> close() override;

    bool isClosed() const override;

    /// Called from network part with close errors
    /// or from close() if is closing by the host
    void close(std::error_code reason);

    // TODO (artem) make RawConnection::id()->string or str() or whatever
    const std::string &str() const {
      return debug_str_;
    }

   private:
    outcome::result<void> saveMultiaddresses();

    boost::asio::io_context &context_;
    Tcp::socket socket_;
    bool initiator_ = false;
    bool connecting_with_timeout_ = false;
    std::atomic_bool connection_phase_done_;
    boost::asio::deadline_timer deadline_timer_;

    /// If true then no more callbacks will be issued
    bool closed_by_host_ = false;

    /// Close reason, is set on close to respond to further calls
    std::error_code close_reason_;

    boost::optional<multi::Multiaddress> remote_multiaddress_;
    boost::optional<multi::Multiaddress> local_multiaddress_;

    friend class security::TlsAdaptor;

    std::string debug_str_;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(libp2p::transport::TcpConnection);
  };
}  // namespace libp2p::transport

#endif  // LIBP2P_TCP_CONNECTION_HPP
