/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/transport/quic/libp2p-quic-api.hpp>

#include <nexus/global_init.hpp>
#include <nexus/quic/client.hpp>
#include <nexus/quic/connection.hpp>
#include <nexus/quic/stream.hpp>

namespace libp2p::security {
  class TlsAdaptor;
}

namespace libp2p::connection {
  class QuicStream;
}

namespace libp2p::transport {
  class QuicTransport;
  class QuicListener;

  /**
   * @brief boost::asio implementation of Quic connection (socket).
   */
  class LIBP2P_QUIC_API QuicConnection
      : public connection::CapableConnection,
        public std::enable_shared_from_this<QuicConnection>,
        private boost::noncopyable {
    friend class connection::QuicStream;
    friend class QuicTransport;
    void setRemoteEndpoint(const multi::Multiaddress &remote);

   public:
    ~QuicConnection() override = default;

    using Udp = boost::asio::ip::udp;
    using ErrorCode = boost::system::error_code;
    using ResolverResultsType = Udp::resolver::results_type;
    using ResolveCallback = void(const ErrorCode &,
                                 const ResolverResultsType &);
    using ResolveCallbackFunc = std::function<ResolveCallback>;
    using ConnectCallback = void(const ErrorCode &, const Udp::endpoint &);
    using ConnectCallbackFunc = std::function<ConnectCallback>;

    QuicConnection(nexus::quic::client &c);
    QuicConnection(nexus::quic::acceptor &a);
    QuicConnection(nexus::quic::client &c,
                   const Udp::endpoint &endpoint,
                   const std::string_view &hostname);

    void start() override;
    void stop() override;
    // initiate stream synchronous
    void newStream(StreamHandlerFunc cb) override;
    // initiate outgoing stream
    outcome::result<std::shared_ptr<libp2p::connection::Stream>> newStream()
        override;
    // set callback for incoming streams
    void onStream(NewStreamHandlerFunc cb)
        override;  // should be renamed to setOnNewStreamHandler() or sth.

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;
    outcome::result<multi::Multiaddress> localMultiaddr() override;

    virtual outcome::result<peer::PeerId> localPeer() const override;
    virtual outcome::result<peer::PeerId> remotePeer() const override;

    virtual outcome::result<crypto::PublicKey> remotePublicKey() const override;

    bool isInitiator() const noexcept override;

    // Closable Interface
    virtual bool isClosed() const override;
    virtual outcome::result<void> close() override;

    // TODO (artem) make RawConnection::id()->string or str() or whatever
    // const std::string &str() const {     return debug_str_;   }

   private:
    // Reader & Writer
    void read(BytesOut out, size_t bytes, ReadCallbackFunc cb) override{};
    void readSome(BytesOut out, size_t bytes, ReadCallbackFunc cb) override{};
    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override{};
    void write(BytesIn in, size_t bytes, WriteCallbackFunc cb) override{};
    void writeSome(BytesIn in, size_t bytes, WriteCallbackFunc cb) override{};
    void deferWriteCallback(std::error_code ec,
                            WriteCallbackFunc cb) override{};

    void accept_streams();
    outcome::result<void> saveMultiaddresses();

    bool m_is_initiator = false;

    /// If true then no more callbacks will be issued
    bool closed_by_host_ = false;

    /// Close reason, is set on close to respond to further calls
    std::error_code close_reason_;

    boost::optional<multi::Multiaddress> remote_multiaddress_;
    boost::optional<multi::Multiaddress> local_multiaddress_;

    std::string debug_str_;

    NewStreamHandlerFunc m_on_stream_cb;

    nexus::quic::connection m_conn;
    log::Logger log_;
    friend class QuicListener;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(libp2p::transport::QuicConnection);
  };
}  // namespace libp2p::transport
