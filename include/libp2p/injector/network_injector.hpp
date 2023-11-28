/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/di.hpp>
#include <string_view>
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <nexus/quic/settings.hpp>

// implementations
#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/crypto/aes_ctr/aes_ctr_impl.hpp>
#include <libp2p/crypto/crypto_provider/crypto_provider_impl.hpp>
#include <libp2p/crypto/ecdsa_provider/ecdsa_provider_impl.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/hmac_provider/hmac_provider_impl.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>
#include <libp2p/crypto/key_validator/key_validator_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/crypto/rsa_provider/rsa_provider_impl.hpp>
#include <libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp>
#include <libp2p/layer/websocket.hpp>
#include <libp2p/muxer/mplex.hpp>
#include <libp2p/muxer/yamux.hpp>
#include <libp2p/network/impl/connection_manager_impl.hpp>
#include <libp2p/network/impl/dialer_impl.hpp>
#include <libp2p/network/impl/dnsaddr_resolver_impl.hpp>
#include <libp2p/network/impl/listener_manager_impl.hpp>
#include <libp2p/network/impl/network_impl.hpp>
#include <libp2p/network/impl/router_impl.hpp>
#include <libp2p/network/impl/transport_manager_impl.hpp>
#include <libp2p/peer/impl/identity_manager_impl.hpp>
#include <libp2p/protocol_muxer/multiselect.hpp>
#include <libp2p/security/noise.hpp>
#include <libp2p/security/plaintext.hpp>
#include <libp2p/security/plaintext/exchange_message_marshaller_impl.hpp>
#include <libp2p/security/secio.hpp>
#include <libp2p/security/secio/exchange_message_marshaller_impl.hpp>
#include <libp2p/security/secio/propose_message_marshaller_impl.hpp>
#include <libp2p/security/tls.hpp>
#include <libp2p/transport/impl/upgrader_impl.hpp>
#include <libp2p/transport/tcp.hpp>
#include <libp2p/transport/quic.hpp>

// clang-format off
/**
 * @file network_injector.hpp
 * @brief This header defines DI injector helpers, which can be used instead of
 * manual wiring.
 *
 * The main function in this header is
 * @code makeNetworkInjector() @endcode
 * Use it to create a Boost.DI container with default types.
 *
 * By default:
 * - TCP is used as transport
 * - Plaintext as security
 * - Yamux as muxer
 * - Random keypair is generated
 *
 * List of libraries that should be linked to your lib/exe:
 *  - libp2p_network
 *  - libp2p_tcp
 *  - libp2p_yamux
 *  - libp2p_plaintext
 *  - libp2p_connection_manager
 *  - libp2p_transport_manager
 *  - libp2p_listener_manager
 *  - libp2p_identity_manager
 *  - libp2p_dialer
 *  - libp2p_router
 *  - multiselect
 *  - random_generator
 *  - key_generator
 *  - marshaller
 *
 * <b>Example 1</b>: Make default network with Yamux as muxer, Plaintext as
 * security, TCP as transport.
 * @code
 * auto injector = makeNetworkInjector();
 * std::shared_ptr<Network> network = injector.create<std::shared_ptr<Network>>();
 * assert(network != nullptr);
 * @endcode
 *
 * <b>Example 2</b>: Make network with new transport, muxer and security.
 * @code
 * struct NewTransport : public TransportAdaptor {...};
 * struct NewMuxer : public MuxerAdaptor {...};
 * struct NewSecurity : public SecurityAdaptor {...};
 *
 * auto injector = makeNetworkInjector(
 *   useTransportAdaptors<NewTransport>(),
 *   useMuxerAdaptors<NewMuxer>(),
 *   useSecurityAdaptors<NewSecurity>()
 * );
 *
 * std::shared_ptr<Network> network = injector.create<std::shared_ptr<Network>>();
 * assert(network != nullptr);
 * @endcode
 *
 * <b>Example 3</b>: Use mocked router:
 * @code
 * struct RouterMock : public Router {...};
 *
 * auto injector = makeNetworkInjector(
 *   boost::di::bind<Router>.to<RouterMock>()
 * );
 *
 * // build network
 * std::shared_ptr<Network> network = injector.create<std::shared_ptr<Network>>();
 * assert(network != nullptr);
 *
 * // get mock
 * std::shared_ptr<RouterMock> routerMock = injector.create<std::shared_ptr<RouterMock>>();
 * assert(routerMock != nullptr);
 * @endcode
 *
 * <b>Example 4</b>: Use instance of mock.
 * @code
 * struct RouterMock : public Router {...};
 *
 * auto routerMock = std::make_shared<RouterMock>();
 *
 * auto injector = makeNetworkInjector(
 *   boost::di::bind<Router>.to(routerMock)
 * );
 *
 * // build network
 * std::shared_ptr<Network> network = injector.create<std::shared_ptr<Network>>();
 * assert(network != nullptr);
 * @endcode
 */

// clang-format on

namespace libp2p::injector {

 /*   BOOST_DI_INJECT(QuicConfig, (named = "ssl_context"_s) const boost::asio::ssl::context &ssl, \
                              (named = "quic_settings"_s ) const nexus::quic::settings& s \
                              (named = "key_path"_s ) const std::string_view& key_path \
                              (named = "cert_path"_s ) const std::string_view& cert_path   ); */

/*    
return di::make_injector(    
   di::bind<nexus::quic::settings>.named("quic_settings"sv).to(s),
   di::bind<std::string_view>.named("key_path"sv).to(key_path),
   di::bind<std::string_view>.named("cert_path"sv).to(cert_path)
);*/


/* this can be use by clients

    use a random generated keypair to create a (unsigned) certificate,
    but load a ca_file to verify the server with
*/
/*
inline auto useQuicConfig(  const std::string_view& ca_path ) // crypto::KeyPair& keypair,
{
std::shared_ptr< libp2p::transport::QuicConfig>  config 
= std::make_shared<libp2p::transport::QuicConfig>(  ca_path); // keypair,

return boost::di::make_injector(
   boost::di::bind<libp2p::transport::QuicConfig>.to( 
    [cfg{std::move(config)}]() ->libp2p::transport::QuicConfig&
   {
      return *cfg;
   } ).in(boost::di::singleton)[boost::di::override] 
);
}
*/


inline auto useQuicConfig(// const nexus::quic::settings& server_settings,
                          // const nexus::quic::settings& client_settings,
                          const std::string_view& key_path,
                          const std::string_view& cert_path,
                          const std::string_view& ca_path
                          )
{using namespace std::literals::string_view_literals;
  using namespace std::string_literals;
  using namespace boost;


return di::make_injector(    
 //  di::bind<nexus::quic::settings>.named( _server_settings).to(server_settings),
//  di::bind<nexus::quic::settings>.named( _client_settings).to(client_settings),
   di::bind<std::string_view>.named(_key_path).to(key_path),
   di::bind<std::string_view>.named(_cert_path).to(cert_path),
   di::bind<std::string_view>.named(_ca_path).to(ca_path)
);

}

/*
inline auto useQuicConfig( const boost::asio::ssl::context &ssl,
                          const nexus::quic::settings& s                          
                          )
{
  using namespace std::string_literals;
  using namespace boost;
  using namespace std::literals::string_view_literals;
 
return di::make_injector(    
  di::bind<asio::ssl::context>.named("ssl_context"s).to(ssl),
   di::bind<nexus::quic::settings>.named("quic_settings"s).to(s)   
);
  }
  */
inline auto get_key_type(EVP_PKEY *pkey) {
    int type = EVP_PKEY_base_id(pkey);

    switch (type) {
        case EVP_PKEY_RSA:
        return libp2p::crypto::Key::Type::RSA;
            break;
        case EVP_PKEY_ED25519:
            return libp2p::crypto::Key::Type::Ed25519;
            break;
        // Add more cases for other key types as needed
        default:
          return libp2p::crypto::Key::Type::UNSPECIFIED;
    }
}


  /**
   * @brief Instruct injector to use this keypair. Can be used once.
   *
   * @code
   * KeyPair keyPair = {...};
   * auto injector = makeNetworkInjector(
   *   useKeyPair(std::move(keyPair))
   * );
   * @endcode
   */
  inline auto useKeyPair(const crypto::KeyPair &key_pair) {
    return boost::di::bind<crypto::KeyPair>().template to(
        key_pair)[boost::di::override];
  }

/*! \brief retrieve private and public key from .pem encoded file

 this can be used by servers
  \param key_path path to private key file
  \param cert_path path to pem encoded x509 certificate containing the public key
  \todo check that key and cert match
*/
  inline auto useKeyPairPEM( const std::string_view& key_path ,
                               const std::string_view& cert_path )
  {
    using cbase= boost::asio::ssl::context_base;
    boost::asio::ssl::context ssl(  cbase::tlsv13 );

  ssl.use_certificate_chain_file(cert_path.data());
  ssl.use_private_key_file(key_path.data(), boost::asio::ssl::context::file_format::pem);


  crypto::KeyPair fileKeys;

// Extract the X509 certificate from the SSL context
    X509 *cert = ::SSL_CTX_get0_certificate(ssl.native_handle() );
    if (!cert) {
      throw std::runtime_error("invalid certificate file");
        // Handle error
        // ERR_print_errors_fp(stderr);
        // SSL_CTX_free(ctx);
        // return 1;
    }

    // Retrieve the public key from the X509 certificate
    EVP_PKEY *public_key = ::X509_get_pubkey(cert);
    if (!public_key) {
      throw std::runtime_error( "invalid certificate file");
        // Handle error
        // ERR_print_errors_fp(stderr);
        // X509_free(cert);
        // SSL_CTX_free(ctx);
        //return 1;
    }
    fileKeys.publicKey.type = get_key_type( public_key);

// Get the raw bytes of the public key
    unsigned char *public_key_bytes = NULL;
    size_t public_key_size = ::i2d_PUBKEY(public_key, &public_key_bytes);
    if( public_key_size)
    {
      fileKeys.publicKey.data.resize(public_key_size,0);
      std::memcpy( fileKeys.publicKey.data.data(), (uint8_t*)public_key_bytes, public_key_size );
    }else
    {

    
    size_t len=0;
    size_t raw_len = ::EVP_PKEY_get_raw_public_key(public_key, NULL, &len);
    if (raw_len > 0) {
        
        fileKeys.publicKey.data.resize(raw_len,0);        
            ::EVP_PKEY_get_raw_public_key(public_key, fileKeys.publicKey.data.data(),&len );
        
    }
    }

    // Retrieve the private key from the context
    EVP_PKEY *private_key = SSL_CTX_get0_privatekey(ssl.native_handle());
    if (!private_key) {
        
        throw std::runtime_error("invalid private key file");
    }

    fileKeys.privateKey.type = get_key_type( private_key );

    // Get the raw bytes of the private key
    const unsigned char *private_key_bytes = (const unsigned char* ) EVP_PKEY_get0(private_key);
    size_t private_key_size = EVP_PKEY_size(private_key);

    fileKeys.privateKey.data.resize(private_key_size,0);
    std::memcpy( fileKeys.privateKey.data.data(), private_key_bytes, private_key_size );



  return boost::di::bind<crypto::KeyPair>().template to( std::move(fileKeys ))[boost::di::override];
  }

  /**
   * @brief Instruct injector to use wss ssl server with key and certificates
   * from pem. Can be used once.
   */
  inline auto useWssPem(std::string_view pem) {
    layer::WssCertificate cert;
    if (not pem.empty()) {
      if (auto cert_res = layer::WssCertificate::make(pem)) {
        cert = std::move(cert_res.value());
      } else {
        SL_WARN(log::createLogger("libp2p::injector::useWssPem"),
                "{}",
                cert_res.error().message());
      }
    }
    return boost::di::bind<layer::WssCertificate>.template to(
        std::move(cert))[boost::di::override];
  }

  /**
   * @brief Instruct injector to use specific config type. Can be used many
   * times for different types.
   * @tparam C config type
   * @param c config instance
   * @return injector binding
   *
   * @code
   * // config definition
   * struct YamuxConfig {
   *   int a = 5;
   * }
   *
   * // config consumer definition
   * struct Yamux {
   *   Yamux(YamuxConfig config);
   * }
   *
   * // create injector
   * auto injector = makeNetworkInjector(
   *   // change default value a=5 to a=3
   *   useConfig<YamuxConfig>({.a = 3})
   * );
   * @endcode
   */
  template <typename C>
  inline auto useConfig(C &&c) {
    return boost::di::bind<std::decay<C>>().template to(
        std::forward<C>(c))[boost::di::override];
  }

  /**
   * @brief Bind layer adaptors by type. Can be used once. Technically many
   * types can be specified, even the same type, but in the end only 1 instance
   * for each type is created.
   * @tparam LayerImpl one or many types of layer adaptors to be used
   * @return injector binding
   *
   * @code
   * struct SomeNewAdaptor : public LayerAdaptor {...};
   *
   * auto injector = makeNetworkInjector(
   *   useLayerAdaptors<WsAdaptor>()
   * );
   * @endcode
   */
  template <typename... AdaptorImpl>
  inline auto useLayerAdaptors() {
    return boost::di::bind<layer::LayerAdaptor *[]>()  // NOLINT
        .template to<AdaptorImpl...>()[boost::di::override];
  }

  /**
   * @brief Bind security adaptors by type. Can be used once. Technically many
   * types can be specified, even the same type, but in the end only 1 instance
   * for each type is created.
   * @tparam SecImpl one or many types of security adaptors to be used
   * @return injector binding
   *
   * @code
   * struct SomeNewAdaptor : public SecurityAdaptor {...};
   *
   * auto injector = makeNetworkInjector(
   *   useSecurityAdaptors<Plaintext, SomeNewAdaptor, SecioAdaptor>()
   * );
   * @endcode
   */
  template <typename... SecImpl>
  inline auto useSecurityAdaptors() {
    return boost::di::bind<security::SecurityAdaptor *[]>()  // NOLINT
        .template to<SecImpl...>()[boost::di::override];
  }

  /**
   * @brief Bind muxer adaptors by types. Can be used once. Technically many
   * types can be specified, even the same type, but in the end only 1 instance
   * for each type is created.
   * @tparam MuxerImpl one or many types of muxer adaptors to be used
   * @return injector binding
   */
  template <typename... MuxerImpl>
  inline auto useMuxerAdaptors() {
    return boost::di::bind<muxer::MuxerAdaptor *[]>()  // NOLINT
        .template to<MuxerImpl...>()[boost::di::override];
  }

  /**
   * @brief Instruct injector to use these transports. Can be used once.
   * Technically many types can be specified, even the same type, but in the end
   * only 1 instance for each type is created.
   * @tparam TransportImpl one or many types of transport adaptors to be used
   * @return injector binding
   */
  template <typename... TransportImpl>
  inline auto useTransportAdaptors() {
    return boost::di::bind<transport::TransportAdaptor *[]>()  // NOLINT
        .template to<TransportImpl...>()[boost::di::override];
  }

  /**
   * @brief Main function that creates Network Injector.
   * @tparam Ts types of injector bindings
   * @param args injector bindings that override default bindings.
   * @return complete network injector
   */
  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  inline auto makeNetworkInjector(Ts &&...args) {
    namespace di = boost::di;

    auto csprng = std::make_shared<crypto::random::BoostRandomGenerator>();
    auto ed25519_provider =
        std::make_shared<crypto::ed25519::Ed25519ProviderImpl>();
    auto rsa_provider = std::make_shared<crypto::rsa::RsaProviderImpl>();
    auto ecdsa_provider = std::make_shared<crypto::ecdsa::EcdsaProviderImpl>();
    auto secp256k1_provider =
        std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>();
    auto hmac_provider = std::make_shared<crypto::hmac::HmacProviderImpl>();
    std::shared_ptr<crypto::CryptoProvider> crypto_provider =
        std::make_shared<crypto::CryptoProviderImpl>(csprng,
                                                     ed25519_provider,
                                                     rsa_provider,
                                                     ecdsa_provider,
                                                     secp256k1_provider,
                                                     hmac_provider);
    auto validator =
        std::make_shared<crypto::validator::KeyValidatorImpl>(crypto_provider);

    // assume no error here. otherwise... just blow up executable
    auto keypair =
        crypto_provider->generateKeys(crypto::Key::Type::Ed25519).value();

    // clang-format off
    return di::make_injector<InjectorConfig>(
        di::bind<crypto::random::RandomGenerator>.template to<crypto::random::BoostRandomGenerator>(),

        di::bind<crypto::KeyPair>().template to(std::move(keypair)),
        di::bind<crypto::random::CSPRNG>().template to(std::move(csprng)),
        di::bind<crypto::ed25519::Ed25519Provider>().template to(std::move(ed25519_provider)),
        di::bind<crypto::rsa::RsaProvider>().template to(std::move(rsa_provider)),
        di::bind<crypto::ecdsa::EcdsaProvider>().template to(std::move(ecdsa_provider)),
        di::bind<crypto::secp256k1::Secp256k1Provider>().template to(std::move(secp256k1_provider)),
        di::bind<crypto::aes::AesCtr>().template to<crypto::aes::AesCtrImpl>(),
        di::bind<crypto::hmac::HmacProvider>().template to<crypto::hmac::HmacProviderImpl>(),
        di::bind<crypto::CryptoProvider>().template to<crypto::CryptoProviderImpl>(),
        di::bind<crypto::marshaller::KeyMarshaller>().template to<crypto::marshaller::KeyMarshallerImpl>(),
        di::bind<peer::IdentityManager>().template to<peer::IdentityManagerImpl>(),
        di::bind<crypto::validator::KeyValidator>().template to<crypto::validator::KeyValidatorImpl>(),
        di::bind<security::plaintext::ExchangeMessageMarshaller>().template to<security::plaintext::ExchangeMessageMarshallerImpl>(),
        di::bind<security::secio::ProposeMessageMarshaller>().template to<security::secio::ProposeMessageMarshallerImpl>(),
        di::bind<security::secio::ExchangeMessageMarshaller>().template to<security::secio::ExchangeMessageMarshallerImpl>(),
        di::bind<layer::WsConnectionConfig>.template to(layer::WsConnectionConfig{}),
        di::bind<layer::WssCertificate>.template to(layer::WssCertificate{}),

        di::bind<basic::Scheduler::Config>.template to(basic::Scheduler::Config{}),
        di::bind<basic::SchedulerBackend>().template to<basic::AsioSchedulerBackend>(),
        di::bind<basic::Scheduler>().template to<basic::SchedulerImpl>(),

        // internal
        di::bind<network::DnsaddrResolver>().template to <network::DnsaddrResolverImpl>(),
        di::bind<network::Router>().template to<network::RouterImpl>(),
        di::bind<network::ConnectionManager>().template to<network::ConnectionManagerImpl>(),
        di::bind<network::ListenerManager>().template to<network::ListenerManagerImpl>(),
        di::bind<network::Dialer>().template to<network::DialerImpl>(),
        di::bind<network::Network>().template to<network::NetworkImpl>(),
        di::bind<network::TransportManager>().template to<network::TransportManagerImpl>(),
        di::bind<transport::Upgrader>().template to<transport::UpgraderImpl>(),
        di::bind<protocol_muxer::ProtocolMuxer>().template to<protocol_muxer::multiselect::Multiselect>(),

        // default adaptors
        di::bind<muxer::MuxedConnectionConfig>.template to(muxer::MuxedConnectionConfig{}),
        di::bind<layer::LayerAdaptor *[]>().template to<layer::WsAdaptor, layer::WssAdaptor>(),  // NOLINT
        di::bind<security::SecurityAdaptor *[]>().template to<security::Plaintext, security::Secio, security::Noise, security::TlsAdaptor>(),  // NOLINT
        di::bind<muxer::MuxerAdaptor *[]>().template to<muxer::Yamux, muxer::Mplex>(),  // NOLINT
        di::bind<transport::TransportAdaptor *[]>().template to<transport::QuicTransport,transport::TcpTransport>(),  // NOLINT

        // user-defined overrides...
        std::forward<decltype(args)>(args)...
    );
    // clang-format on
  }

}  // namespace libp2p::injector
