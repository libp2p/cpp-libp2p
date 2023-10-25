/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <memory>
#include <string>

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

struct SecureAdaptorProxy : libp2p::security::SecurityAdaptor {
  libp2p::peer::ProtocolName getProtocolId() const override {
    return impl->getProtocolId();
  }

  void secureInbound(
      std::shared_ptr<libp2p::connection::LayerConnection> inbound,
      SecConnCallbackFunc cb) override {
    return impl->secureInbound(inbound, cb);
  }

  void secureOutbound(
      std::shared_ptr<libp2p::connection::LayerConnection> outbound,
      const libp2p::peer::PeerId &p, SecConnCallbackFunc cb) override {
    return impl->secureOutbound(outbound, p, cb);
  }

  std::shared_ptr<libp2p::security::SecurityAdaptor> impl;
};

int main(int argc, char **argv) {
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::common::operator""_unhex;

  auto has_arg = [&](std::string_view arg) {
    auto args = std::span(argv, argc).subspan(1);
    return std::find(args.begin(), args.end(), arg) != args.end();
  };

  if (has_arg("-h") or has_arg("--help")) {
    fmt::print("Options:\n");
    fmt::print("  -h, --help\n");
    fmt::print("    Print help\n");
    fmt::print("  -insecure\n");
    fmt::print("    Use plaintext protocol instead of noise\n");
    fmt::print("  --ws\n");
    fmt::print("    Accept websocket connections instead of tcp\n");
    fmt::print("  --wss\n");
    fmt::print("    Accept secure websocket connections instead of tcp\n");
    return 0;
  }

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

  bool insecure_mode{has_arg("-insecure")};
  if (insecure_mode) {
    log->info("Starting in insecure mode");
  } else {
    log->info("Starting in secure mode");
  }

  auto injector = libp2p::injector::makeHostInjector(
      libp2p::injector::useKeyPair(keypair),
      libp2p::injector::useSecurityAdaptors<SecureAdaptorProxy>(),
      libp2p::injector::useWssPem(R"(
-----BEGIN CERTIFICATE-----
MIIBODCB3qADAgECAghv+C53VY1w3TAKBggqhkjOPQQDAjAUMRIwEAYDVQQDDAls
b2NhbGhvc3QwIBcNNzUwMTAxMDAwMDAwWhgPNDA5NjAxMDEwMDAwMDBaMBQxEjAQ
BgNVBAMMCWxvY2FsaG9zdDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABLNFvFLB
kzZEhSjaSNnS5Q+364BqSLF0+2x7gZVEDazBtdxlfmIVWL9Xymgil1WuCfmIxp2R
Cdh/0A9Ym4Zx5sqjGDAWMBQGA1UdEQQNMAuCCWxvY2FsaG9zdDAKBggqhkjOPQQD
AgNJADBGAiEAnfqMaHg9KVCbg1OHmZ19f7ArfwNLj5fmTFB3OYeisycCIQCg2rDy
MLbRdSECggJ2ae10PIutrY7c+78h1vHDfXRM7A==
-----END CERTIFICATE-----

-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgdfUHplIKKrgBaZUd
FVg0biAiKZmXu+iWX43vprg2c/ShRANCAASzRbxSwZM2RIUo2kjZ0uUPt+uAakix
dPtse4GVRA2swbXcZX5iFVi/V8poIpdVrgn5iMadkQnYf9APWJuGcebK
-----END PRIVATE KEY-----
)"));
  auto &secure_adaptor = injector.create<SecureAdaptorProxy &>();
  if (insecure_mode) {
    secure_adaptor.impl =
        injector.create<std::shared_ptr<libp2p::security::Plaintext>>();
  } else {
    secure_adaptor.impl =
        injector.create<std::shared_ptr<libp2p::security::Noise>>();
  }
  auto host = injector.create<std::shared_ptr<libp2p::Host>>();
  auto io_context = injector.create<std::shared_ptr<boost::asio::io_context>>();

  // set a handler for Echo protocol
  libp2p::protocol::Echo echo{libp2p::protocol::EchoConfig{
      .max_server_repeats =
          libp2p::protocol::EchoConfig::kInfiniteNumberOfRepeats,
      .max_recv_size =
          libp2p::muxer::MuxedConnectionConfig::kDefaultMaxWindowSize}};
  host->setProtocolHandler({echo.getProtocolId()},
                           [&echo](libp2p::StreamAndProtocol stream) {
                             echo.handle(std::move(stream));
                           });

  std::string _ma = "/ip4/127.0.0.1/tcp/40010";
  if (has_arg("--wss")) {
    _ma += "/wss";
  } else if (has_arg("--ws")) {
    _ma += "/ws";
  }
  auto ma = libp2p::multi::Multiaddress::create(_ma).value();

  // launch a Listener part of the Host
  io_context->post([&] {
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
    io_context->run();
    std::exit(EXIT_SUCCESS);
  } catch (const boost::system::error_code &ec) {
    log->error("Server cannot run: {}", ec.message());
    std::exit(EXIT_FAILURE);
  } catch (...) {
    log->error("Unknown error happened");
    std::exit(EXIT_FAILURE);
  }
}
