/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <memory>

#include <soralog/injector.hpp>

#include <libp2p/common/literals.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/protocol/echo.hpp>

namespace {
  template <typename Injector>
  std::shared_ptr<soralog::Configurator> get_logging_system_configurator(
      Injector &injector) {
    static boost::optional<std::shared_ptr<soralog::ConfiguratorFromYAML>>
        instance;
    if (instance.has_value()) {
      return *instance;
    }

    auto libp2p_log_cfg =
        injector.template create<std::shared_ptr<libp2p::log::Configurator>>();

    instance = std::make_shared<soralog::ConfiguratorFromYAML>(
        std::move(libp2p_log_cfg), std::string(R"(
# ----------------
sinks:
  - name: console
    type: console
    color: true
groups:
  - name: main
    sink: console
    level: info
    children
      - name: libp2p
# ----------------
  )"));

    return *instance;
  }
}  // namespace

int main(int argc, char *argv[]) {
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::common::operator""_unhex;

  if (argc != 2) {
    std::cerr << "please, provide an address of the server\n";
    std::exit(EXIT_FAILURE);
  }

  // create Echo protocol object - it implement the logic of both server and
  // client, but in this example it's used as a client-only
  libp2p::protocol::Echo echo{libp2p::protocol::EchoConfig{1}};

  // create a default Host via an injector
  auto injector = libp2p::injector::makeHostInjector();

  // prepare log system
  auto logger_injector = soralog::injector::makeInjector(
      boost::di::bind<soralog::Configurator>.to([&injector](const auto &i) {
        return get_logging_system_configurator(injector);
      })[boost::di::override]);
  auto logger_system =
      logger_injector.create<std::shared_ptr<soralog::LoggerSystem>>();
  auto r = logger_system->configure();
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }

  libp2p::log::setLoggerSystem(logger_system);
  if (std::getenv("TRACE_DEBUG") != nullptr) {
    libp2p::log::setLevelOfGroup("*", soralog::Level::TRACE);
  } else {
    libp2p::log::setLevelOfGroup("*", soralog::Level::ERROR);
  }

  auto host = injector.create<std::shared_ptr<libp2p::Host>>();

  // create io_context - in fact, thing, which allows us to execute async
  // operations
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  context->post([host{std::move(host)}, &echo, argv] {  // NOLINT
    auto server_ma_res =
        libp2p::multi::Multiaddress::create(argv[1]);  // NOLINT
    if (!server_ma_res) {
      std::cerr << "unable to create server multiaddress: "
                << server_ma_res.error().message() << std::endl;
      std::exit(EXIT_FAILURE);
    }
    auto server_ma = std::move(server_ma_res.value());

    auto server_peer_id_str = server_ma.getPeerId();
    if (!server_peer_id_str) {
      std::cerr << "unable to get peer id" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    auto server_peer_id_res =
        libp2p::peer::PeerId::fromBase58(*server_peer_id_str);
    if (!server_peer_id_res) {
      std::cerr << "Unable to decode peer id from base 58: "
                << server_peer_id_res.error().message() << std::endl;
      std::exit(EXIT_FAILURE);
    }

    auto server_peer_id = std::move(server_peer_id_res.value());

    auto peer_info = libp2p::peer::PeerInfo{server_peer_id, {server_ma}};

    // create Host object and open a stream through it
    host->newStream(
        peer_info, echo.getProtocolId(), [&echo](auto &&stream_res) {
          if (!stream_res) {
            std::cerr << "Cannot connect to server: "
                      << stream_res.error().message() << std::endl;
            std::exit(EXIT_FAILURE);
          }
          auto stream_p = std::move(stream_res.value());

          auto echo_client = echo.createClient(stream_p);
          std::cout << "SENDING 'Hello from C++!'\n";
          echo_client->sendAnd(
              "Hello from C++!\n",
              [stream = std::move(stream_p)](auto &&response_result) {
                std::cout << "RESPONSE " << response_result.value()
                          << std::endl;
                stream->close([](auto &&) { std::exit(EXIT_SUCCESS); });
              });
        });
  });

  // run the IO context
  context->run_for(std::chrono::seconds(5));
}
