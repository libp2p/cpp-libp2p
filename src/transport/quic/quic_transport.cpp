/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#define TRACE_ENABLED 1
#include <libp2p/common/trace.hpp>
#include <libp2p/injector/network_injector.hpp>
#include <libp2p/transport/quic/quic_transport.hpp>
#include <libp2p/log/logger.hpp>
#include "../../security/tls/tls_details.hpp"




// #include <libp2p/crypto/key_marshaller.hpp>

// #include <libp2p/transport/impl/upgrader_session.hpp>

namespace libp2p::transport {

  int alpn_select_cb(SSL *ssl,
                     const unsigned char **out,
                     unsigned char *outlen,
                     const unsigned char *in,
                     unsigned int inlen,
                     void *arg) {
    /*
     unsigned char vector[] = {
       6, 's', 'p', 'd', 'y', '/', '1',
       8, 'h', 't', 't', 'p', '/', '1', '.', '1'
   };
   unsigned int length = sizeof(vector);
    */
    const unsigned char alpn[] = {4, 'e', 'c', 'h', 'o'};
    int r = ::SSL_select_next_proto(const_cast<unsigned char **>(out),
                                    outlen,
                                    const_cast<unsigned char *>(in),
                                    inlen,
                                    alpn,
                                    sizeof(alpn));
    if (r == OPENSSL_NPN_NEGOTIATED) {
      return SSL_TLSEXT_ERR_OK;
    } else {
      return SSL_TLSEXT_ERR_ALERT_FATAL;
    }
  }

  /*
    QuicConfig::QuicConfig( const boost::asio::ssl::context &ssl,
                                 const nexus::quic::settings& s
                             )
                                 {

                                 }
  */

  using cbase = boost::asio::ssl::context_base;

  void QuicConfig::def_ctor_impl() {
    
    ::SSL_CTX_set_alpn_select_cb(ssl.native_handle(), alpn_select_cb, nullptr);
    const unsigned char alpn[] = {4, 'e', 'c', 'h', 'o'};
    ::SSL_CTX_set_alpn_protos(ssl.native_handle(), alpn, sizeof(alpn));
    

    ssl.set_options(cbase::no_compression | cbase::no_sslv2 | cbase::no_sslv3
                    | cbase::no_tlsv1_1 | cbase::no_tlsv1_2); 

     ssl.set_verify_mode(boost::asio::ssl::verify_peer
                         | boost::asio::ssl::verify_fail_if_no_peer_cert
                         | boost::asio::ssl::verify_client_once);
  // ssl.set_verify_mode(boost::asio::ssl::verify_none);

 //   ssl.set_verify_callback(&libp2p::security::tls_details::verifyCallback);



    auto injector = libp2p::injector::makeNetworkInjector();

   [[maybe_unused]] auto ck = libp2p::security::tls_details::makeCertificate(
        injector.create<crypto::KeyPair>(), // this will yield a random generated keypair for clients
                                          // or the one that was specified with useKeyPair() i.e. for servers
        *injector.create<std::shared_ptr<crypto::marshaller::KeyMarshaller> >());

             

    ssl.use_certificate(
        boost::asio::const_buffer(ck.certificate.data(), ck.certificate.size()),
        boost::asio::ssl::context_base::asn1);

    ssl.use_private_key(
        boost::asio::const_buffer(ck.private_key.data(), ck.private_key.size()),
        boost::asio::ssl::context_base::asn1);
  }

  QuicConfig::QuicConfig() : ssl(cbase::tlsv13) {
    def_ctor_impl();
  }

  /*
   this constructor can be used by clients
   use a random generated keypair to create a (unsigned) certificate.
  But load a ca_cert to verify the server certificate with.
  */
  /*
   QuicConfig::QuicConfig(// crypto::KeyPair& keypair,
                          const std::string_view &ca_path)
       : ssl(cbase::tlsv13) {
     ssl.load_verify_file(ca_path.data());

     def_ctor_impl();
   }
 */

  /*!
    \param  key_path filepath to private key in ASN1 or Pem format
    \param  cert_path filepath to certificate in .pem format
    \param ca_path path of a file containing certification authority
    certificates in PEM format
  */
  /*
   QuicConfig::QuicConfig(const nexus::quic::settings &_server_settings,
                          const nexus::quic::settings &_client_settings,
                          const std::string_view &key_path,
                          const std::string_view &cert_path,
                          const std::string_view &ca_path)
       : ssl(cbase::tlsv13),
         client_settings(_client_settings),
         server_settings(_server_settings)
         {

     //ssl.use_certificate_chain_file(cert_path.data());
     //ssl.use_private_key_file(key_path.data(),
     //boost::asio::ssl::context::file_format::pem);
     // ssl.load_verify_file(ca_path.data() );

     ssl.set_options(cbase::no_compression | cbase::no_sslv2 | cbase::no_sslv3
                     | cbase::no_tlsv1_1 | cbase::no_tlsv1_2);
     ::SSL_CTX_set_alpn_select_cb(ssl.native_handle(), alpn_select_cb, nullptr);
     const unsigned char alpn[] = {4, 'e', 'c', 'h', 'o'};
     ::SSL_CTX_set_alpn_protos(ssl.native_handle(), alpn, sizeof(alpn));

         }
         */

  // const char* hostname = "maybe_sth_from_my_pubkey_here"; // max. 255 chars
  // SSL_set_tlsext_host_name( ssl.native_handle(), hostname );
  /*
      try {
        ssl_context_ =
     std::make_shared<boost::asio::ssl::context>(cbase::tlsv13);

        ssl_context_->set_verify_mode(
            boost::asio::ssl::verify_peer
            | boost::asio::ssl::verify_fail_if_no_peer_cert
            | boost::asio::ssl::verify_client_once);
        ssl_context_->set_verify_callback(&tls_details::verifyCallback);

        auto ck =
            tls_details::makeCertificate(idmgr_->getKeyPair(),
     *key_marshaller_);

        ssl_context_->use_certificate(
            boost::asio::const_buffer(ck.certificate.data(),
                                      ck.certificate.size()),
            boost::asio::ssl::context_base::asn1);

        ssl_context_->use_private_key(
            boost::asio::const_buffer(ck.private_key.data(),
                                      ck.private_key.size()),
            boost::asio::ssl::context_base::asn1);
      } catch (const std::exception &e) {
        ssl_context_.reset();
        log()->error("context init failed: {}", e.what());
        return TlsError::TLS_CTX_INIT_FAILED;
      }
  */

  /*QuicTransport::QuicTransport(std::shared_ptr<boost::asio::io_context>
     context                         ) : context_(std::move(context)) {}
      */

  /*
  - maybe use separate quic::settings for client and server ?!
  */
  QuicTransport::QuicTransport(std::shared_ptr<boost::asio::io_context> context
                                // ,   QuicConfig &&config
  )
      // const Udp::Endpoint &local_endpoint )
      //: m_client(context.get_executor(), local_endpoint, ssl, s),
      : context_(context),
      //  m_config(std::move(config)),
        m_nexus_ctx(nexus::global::init_client_server())
        , m_clients{nexus::quic::client(context->get_executor(),
                                      Udp::endpoint{Udp::v4(), 0},
                                      m_config.ssl
                                       , m_config.client_settings
                                      )
                                      ,
                  nexus::quic::client(context->get_executor(),
                                      Udp::endpoint{Udp::v6(), 0},
                                      m_config.ssl
                                       , m_config.client_settings
                                      )                 
                                      },
        m_server(context->get_executor(), m_config.server_settings) ,
        log_(log::createLogger("QuicTransport"))
        {
          SL_TRACE(log_, "client local addr:  {}:{} {}:{}",
           m_clients[0].local_endpoint().address().to_string(),
           m_clients[0].local_endpoint().port(),
           m_clients[1].local_endpoint().address().to_string(),
           m_clients[1].local_endpoint().port()
            );
          m_nexus_ctx.log_to_stderr("debug");
        }

  void QuicTransport::dial(const peer::PeerId &remoteId,
                           multi::Multiaddress address,
                           TransportAdaptor::HandlerFunc handler) {
    dial(remoteId,
         std::move(address),
         std::move(handler),
         std::chrono::milliseconds::zero());
  }

  void QuicTransport::dial(const peer::PeerId &remoteId,
                           multi::Multiaddress address,
                           TransportAdaptor::HandlerFunc handler,
                           std::chrono::milliseconds timeout) {
    if (!canDial(address)) {
      // TODO(107): Reentrancy

      return handler(std::errc::address_family_not_supported);
    }

    auto [host, port] = detail::getHostAndPort(address);

    auto layers = detail::getLayers(address);

    // gets called, once address resolution is done
    auto connect = [this,
                    address,
                    hostname = host,
                    handler{std::move(handler)},
                    layers = std::move(layers)](auto ec, auto r) mutable {
      // resolve failed
      if (ec) {
        return handler(ec);
      }

      /// open a connection to the given remote endpoint and hostname. this
      /// initiates the TLS handshake, but returns immediately without waiting
      /// for the handshake to complete
      const auto &endp = (*r).endpoint();

      SL_TRACE(log_, "client dialing: {}:{}", endp.address().to_string(), endp.port() );

      auto conn = std::make_shared<QuicConnection>(
          clientByFamily(endp.protocol().family()), endp, hostname.data());

      conn->setRemoteEndpoint( std::move(address) ); // workaround ^^

      m_conns.emplace_back(conn);
      conn->start();
      handler(conn);

      /* // no need to build manual timeout -> see
         nexus::settings::handshake_timeout
          conn->connect( r,
           [=, handler{std::move(handler)},
                      layers = std::move(layers)]( auto ec, auto &e) mutable
           {
             // connection handshake timeout
             if (ec) {
               std::ignore = conn->close();
               return handler(ec);
             }


           },
           timeout); */
    };

    using P = multi::Protocol::Code;
    switch (detail::getFirstProtocol(address)) {
      case P::DNS4:
        return resolve(boost::asio::ip::udp::v4(), host, port, connect);
      case P::DNS6:
        return resolve(boost::asio::ip::udp::v6(), host, port, connect);
      default:  // Could be only DNS, IP6 or IP4 as canDial already checked for
                // that in the beginning of the method
        return resolve(host, port, std::move(connect) );
    }
  }

  std::shared_ptr<TransportListener> QuicTransport::createListener(
      TransportListener::HandlerFunc handler) {
    return std::make_shared<QuicListener>(
        m_server, m_config.ssl, std::move(handler));
  }

  bool QuicTransport::canDial(const multi::Multiaddress &ma) const {
    return detail::supportsIpQuic(ma);
  }

  peer::ProtocolName QuicTransport::getProtocolId() const {
    return "/quic/1.0.0";
  }

  void QuicTransport::resolve(const Udp::endpoint &endpoint,
                              ResolveCallbackFunc cb) {
    auto resolver = std::make_shared<Udp::resolver>(*context_);
    resolver->async_resolve(
        endpoint,
        [resolver, cb{std::move(cb)}](const ErrorCode &ec, auto &&iterator) {
          cb(ec, std::forward<decltype(iterator)>(iterator));
        });
  }

  void QuicTransport::resolve(const std::string &host_name,
                              const std::string &port,
                              ResolveCallbackFunc cb) {
    auto resolver = std::make_shared<Udp::resolver>(context_->get_executor());

    // commented out, because it was never called. Executor just ran out of work and finished.
    resolver->async_resolve(
        host_name,
        port,
        [ res{resolver}, cb{std::move(cb)},this](const ErrorCode &ec, auto &&iterator) 
        {
          SL_TRACE(log_, "done resolving! err: {}" , ec.message() );
          
          cb(ec, std::forward<decltype(iterator)>(iterator));
        }
        );
        // sync workaround
        // cb( {}, resolver->resolve(host_name,port) );
  }

  void QuicTransport::resolve(const Udp &protocol,
                              const std::string &host_name,
                              const std::string &port,
                              ResolveCallbackFunc cb) {
    auto resolver = std::make_shared<Udp::resolver>(*context_);
    resolver->async_resolve(
        protocol,
        host_name,
        port,
        [resolver, cb{std::move(cb)}](const ErrorCode &ec, auto &&iterator)
         {
          cb(ec, std::forward<decltype(iterator)>(iterator));
        });
  }

}  // namespace libp2p::transport
