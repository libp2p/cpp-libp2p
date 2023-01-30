/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <memory>
#include <string>

#include <gsl/multi_span>
#include <libp2p/common/literals.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/layer/websocket.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/protocol/echo.hpp>
#include <libp2p/security/noise.hpp>
#include <libp2p/security/plaintext.hpp>

namespace {
  const std::string logger_config(R"(
# ----------------
sinks:
 - name: console
   type: console
   color: true
groups:
 - name: main
   sink: console
   level: info
   children:
     - name: libp2p
# ----------------
 )");
}  // namespace

bool isInsecure(int argc, char **argv) {
  const std::string flag{"-insecure"};
  auto args = gsl::multi_span<char *>(argv, argc);
  return std::find(args.begin(), args.end(), flag) != args.end();
}

bool isWebsocket(int argc, char **argv) {
  const std::string flag{"-websocket"};
  auto args = gsl::multi_span<char *>(argv, argc);
  return std::find(args.begin(), args.end(), flag) != args.end();
}

struct ServerContext {
  std::shared_ptr<libp2p::Host> host;
  std::shared_ptr<boost::asio::io_context> io_context;
};

ServerContext initSecureServer(const libp2p::crypto::KeyPair &keypair) {
  auto injector = libp2p::injector::makeHostInjector(
      libp2p::injector::useLayerAdaptors<libp2p::layer::WsAdaptor>(),
      libp2p::injector::useKeyPair(keypair),
      libp2p::injector::useSecurityAdaptors<libp2p::security::Noise>());
  auto host = injector.create<std::shared_ptr<libp2p::Host>>();
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  return {.host = host, .io_context = context};
}

ServerContext initInsecureServer(const libp2p::crypto::KeyPair &keypair) {
  auto injector = libp2p::injector::makeHostInjector(
      libp2p::injector::useLayerAdaptors<libp2p::layer::WsAdaptor>(),
      libp2p::injector::useKeyPair(keypair),
      libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());
  auto host = injector.create<std::shared_ptr<libp2p::Host>>();
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  return {.host = host, .io_context = context};
}

int main(int argc, char **argv) {
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::common::operator""_unhex;

  // prepare log system
  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<soralog::ConfiguratorFromYAML>(
          // Original LibP2P logging config
          std::make_shared<libp2p::log::Configurator>(),
          // Additional logging config for application
          logger_config));
  auto r = logging_system->configure();
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }

  libp2p::log::setLoggingSystem(logging_system);
  if (std::getenv("TRACE_DEBUG") != nullptr) {
    libp2p::log::setLevelOfGroup("main", soralog::Level::TRACE);
  } else {
    libp2p::log::setLevelOfGroup("main", soralog::Level::INFO);
  }

  auto log = libp2p::log::createLogger("EchoServer");

  // resulting PeerId should be
  // 12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV
  KeyPair keypair{PublicKey{{Key::Type::Ed25519,
                             "48453469c62f4885373099421a7365520b5ffb"
                             "0d93726c124166be4b81d852e6"_unhex}},
                  PrivateKey{{Key::Type::Ed25519,
                              "4a9361c525840f7086b893d584ebbe475b4ec"
                              "7069951d2e897e8bceb0a3f35ce"_unhex}}};

  bool insecure_mode{isInsecure(argc, argv)};
  if (insecure_mode) {
    log->info("Starting in insecure mode");
  } else {
    log->info("Starting in secure mode");
  }

  bool ws_mode{isWebsocket(argc, argv)};
  if (ws_mode) {
    log->info("Using websocket layer");
  }

  // create a default Host via an injector, overriding a random-generated
  // keypair with ours
  ServerContext server =
      insecure_mode ? initInsecureServer(keypair) : initSecureServer(keypair);

  // set a handler for Echo protocol
  libp2p::protocol::Echo echo{libp2p::protocol::EchoConfig{
      .max_server_repeats =
          libp2p::protocol::EchoConfig::kInfiniteNumberOfRepeats,
      .max_recv_size =
          libp2p::muxer::MuxedConnectionConfig::kDefaultMaxWindowSize}};
  server.host->setProtocolHandler({echo.getProtocolId()},
                                  [&echo](libp2p::StreamAndProtocol stream) {
                                    echo.handle(std::move(stream));
                                  });

  auto ma =
      libp2p::multi::Multiaddress::create(
          ws_mode ? "/ip4/127.0.0.1/tcp/40010/ws" : "/ip4/127.0.0.1/tcp/40010")
          .value();

  // launch a Listener part of the Host
  server.io_context->post([host{std::move(server.host)}, &ma, &log] {
    auto listen_res = host->listen(ma);
    if (!listen_res) {
      log->error("host cannot listen the given multiaddress: {}",
                 listen_res.error().message());
      std::exit(EXIT_FAILURE);
    }

    host->start();
    log->info("Server started");
    log->info("Listening on: {}", ma.getStringAddress());
    log->info("Peer id: {}", host->getPeerInfo().id.toBase58());
    log->info("Connection string: {}/p2p/{}", ma.getStringAddress(),
              host->getPeerInfo().id.toBase58());
  });

  // run the IO context
  try {
    server.io_context->run();
    std::exit(EXIT_SUCCESS);
  } catch (const boost::system::error_code &ec) {
    log->error("Server cannot run: {}", ec.message());
    std::exit(EXIT_FAILURE);
  } catch (...) {
    log->error("Unknown error happened");
    std::exit(EXIT_FAILURE);
  }
}
