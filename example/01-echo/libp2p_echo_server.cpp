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
  if (2 == argc) {
    const std::string insecure{"-insecure"};
    auto args = gsl::multi_span<char *>(argv, argc);
    if (insecure == args[1]) {
      return true;
    }
  }
  return false;
}

struct ServerContext {
  std::shared_ptr<libp2p::Host> host;
  std::shared_ptr<boost::asio::io_context> io_context;
};

ServerContext initSecureServer(const libp2p::crypto::KeyPair &keypair) {
  auto injector = libp2p::injector::makeHostInjector(
      libp2p::injector::useKeyPair(keypair),
      libp2p::injector::useSecurityAdaptors<libp2p::security::Noise>());
  auto host = injector.create<std::shared_ptr<libp2p::Host>>();
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  return {.host = host, .io_context = context};
}

ServerContext initInsecureServer(const libp2p::crypto::KeyPair &keypair) {
  auto injector = libp2p::injector::makeHostInjector(
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
    libp2p::log::setLevelOfGroup("main", soralog::Level::ERROR);
  }

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
    std::cout << "Starting in insecure mode" << std::endl;
  } else {
    std::cout << "Starting in secure mode" << std::endl;
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
  server.host->setProtocolHandler(
      echo.getProtocolId(),
      [&echo](std::shared_ptr<libp2p::connection::Stream> received_stream) {
        echo.handle(std::move(received_stream));
      });

  // launch a Listener part of the Host
  server.io_context->post([host{std::move(server.host)}] {
    auto ma =
        libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/40010").value();
    auto listen_res = host->listen(ma);
    if (!listen_res) {
      std::cerr << "host cannot listen the given multiaddress: "
                << listen_res.error().message() << "\n";
      std::exit(EXIT_FAILURE);
    }

    host->start();
    std::cout << "Server started\nListening on: " << ma.getStringAddress()
              << "\nPeer id: " << host->getPeerInfo().id.toBase58()
              << std::endl;
    std::cout << "Connection string: " << ma.getStringAddress() << "/ipfs/"
              << host->getPeerInfo().id.toBase58() << std::endl;
  });

  // run the IO context
  try {
    server.io_context->run();
  } catch (const boost::system::error_code &ec) {
    std::cout << "Server cannot run: " << ec.message() << std::endl;
  } catch (...) {
    std::cout << "Unknown error happened" << std::endl;
  }
}
