/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/di.hpp>
#include <libp2p/transport/impl/util.hpp>
#include <libp2p/transport/quic/libp2p-quic-api.hpp>
#include <libp2p/transport/quic/quic_connection.hpp>
#include <libp2p/transport/quic/quic_listener.hpp>
#include <libp2p/transport/transport_adaptor.hpp>

#include <memory>
#include <nexus/global_init.hpp>
#include <nexus/quic/client.hpp>
#include <nexus/quic/server.hpp>
#include <nexus/ssl.hpp>
#include <vector>

inline auto _quic_settings = []() {};
inline auto _server_settings = []() {};
inline auto _client_settings = []() {};
inline auto _ssl_context = []() {};
inline auto _key_path = []() {};
inline auto _cert_path = []() {};
inline auto _ca_path = []() {};

namespace libp2p::transport {
  class QuicTransport;
  // class QuicConnection;
  using namespace std::string_literals;
  using namespace std::literals::string_view_literals;
  class LIBP2P_QUIC_API QuicConfig {
    friend class QuicTransport;
    void def_ctor_impl();
    boost::asio::ssl::context ssl;
    nexus::quic::settings client_settings =
        nexus::quic::default_client_settings();
    nexus::quic::settings server_settings =
        nexus::quic::default_server_settings();

   public:
    /*BOOST_DI_INJECT(QuicConfig, (named = "ssl_context"sv ) const
       boost::asio::ssl::context &ssl, \ (named = "quic_settings"sv ) const
       nexus::quic::settings& s  ); */

    /*  BOOST_DI_INJECT(QuicConfig, (named = "quic_settings"sv ) const
       nexus::quic::settings& s, \
                                  (named = "key_path"sv ) const
       std::string_view& key_path, \ (named = "cert_path"sv ) const
       std::string_view& cert_path   );   */
    QuicConfig();

    /*BOOST_DI_INJECT(QuicConfig, (named = _server_settings ) const
       nexus::quic::settings& , \
                                (named = _client_settings ) const
       nexus::quic::settings& , \
                                  (named = _key_path ) const std::string_view&
       key_path="", \
                                  (named = _cert_path ) const std::string_view&
       cert_path="", \ (named = _ca_path ) const std::string_view& ca_path=""
                                   );      */

    QuicConfig(QuicConfig &&other) = default;

    // QuicConfig( /* crypto::KeyPair& , */const std::string_view& ca_path );
  };

  /**
   * @brief QUIC Transport implementation
   */
  class LIBP2P_QUIC_API QuicTransport
      : public TransportAdaptor,
        public std::enable_shared_from_this<QuicTransport> {
   public:
    using Udp = boost::asio::ip::udp;
    using ErrorCode = boost::system::error_code;
    using ResolverResultsType = Udp::resolver::results_type;
    using ResolveCallback = void(const ErrorCode &,
                                 const ResolverResultsType &);
    using ResolveCallbackFunc = std::function<ResolveCallback>;

    ~QuicTransport() override = default;

    QuicTransport(std::shared_ptr<boost::asio::io_context> context
                  // , QuicConfig&& config
    );

    void dial(const peer::PeerId &remoteId,
              multi::Multiaddress address,
              TransportAdaptor::HandlerFunc handler) override;

    void dial(const peer::PeerId &remoteId,
              multi::Multiaddress address,
              TransportAdaptor::HandlerFunc handler,
              std::chrono::milliseconds timeout) override;

    std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) override;

    bool canDial(const multi::Multiaddress &ma) const override;

    peer::ProtocolName getProtocolId() const override;

   private:
    // the resolve methods are duplicated between quic-transport and
    // tcp-connection they could be moved to Transport-adapter and shared
    /**
     * @brief Resolve service name (DNS).
     * @param endpoint endpoint to resolve.
     * @param cb callback executed on operation completion.
     */
    void resolve(const Udp::endpoint &endpoint, ResolveCallbackFunc cb);

    /**
     * @brief Resolve service name (DNS).
     * @param host_name host name to resolve
     * @param cb callback executed on operation completion.
     */
    void resolve(const std::string &host_name,
                 const std::string &port,
                 ResolveCallbackFunc cb);

    /**
     * @brief Resolve service name (DNS).
     * @param protocol is either Tcp::ip4 or Tcp::ip6 protocol
     * @param host_name host name to resolve
     * @param cb callback executed on operation completion.
     */
    void resolve(const Udp &protocol,
                 const std::string &host_name,
                 const std::string &port,
                 ResolveCallbackFunc cb);

    std::shared_ptr<boost::asio::io_context> context_;

    QuicConfig m_config;

    nexus::global::context m_nexus_ctx;

    nexus::quic::client &clientByFamily(int family) {
      assert(family == Udp::v4().family() || family == Udp::v6().family());
      return (family == Udp::v4().family()) ? m_clients[0] : m_clients[1];
    }
    nexus::quic::client m_clients[2];  // one client for each family IPv4/6
    nexus::quic::server m_server;

    std::vector<std::shared_ptr<QuicConnection> > m_conns;

    log::Logger log_;

  };  // namespace libp2p::transport

}  // namespace libp2p::transport
