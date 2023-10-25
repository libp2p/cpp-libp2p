/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <memory>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/literals.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/layer/websocket/ws_adaptor.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/protocol/echo.hpp>

namespace {
  const std::string logger_config(R"(
# ----------------
sinks:
  - name: console
    type: console
    color: true
    latency: 0
groups:
  - name: main
    sink: console
    level: info
    children:
      - name: libp2p
# ----------------
  )");
}  // namespace

int main(int argc, char *argv[]) {
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::common::operator""_unhex;

  auto run_duration = std::chrono::seconds(5);

  std::string message("Hello from C++");

  if (argc > 2) {
    auto n = atoi(argv[2]);         // NOLINT
    if (n > (int)message.size()) {  // NOLINT
      std::string jumbo_message;
      auto sz = static_cast<size_t>(n);
      jumbo_message.reserve(sz + 9);
      while (jumbo_message.size() < sz) {
        jumbo_message.append(
            fmt::format("[{:08}]", jumbo_message.size() + 10));
      }
      jumbo_message.resize(sz);
      message.swap(jumbo_message);
      run_duration = std::chrono::seconds(150);
    }
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

  auto log = libp2p::log::createLogger("EchoClient");

  if (argc < 2) {
    log->critical("Address of server was not provided");
    log->info("Please, provide an address of the server");
    std::exit(EXIT_FAILURE);
  }

  // create Echo protocol object - it implement the logic of both server and
  // client, but in this example it's used as a client-only
  libp2p::protocol::Echo echo{libp2p::protocol::EchoConfig{1}};

  // create a default Host via an injector
  auto injector = libp2p::injector::makeHostInjector();

  auto host = injector.create<std::shared_ptr<libp2p::Host>>();

  // create io_context - in fact, thing, which allows us to execute async
  // operations
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  auto sch = injector.create<std::shared_ptr<libp2p::basic::Scheduler>>();

  context->post(
      [log, host{std::move(host)}, &echo, &message,
       argv,  // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
       sch] {
        auto server_ma_res =
            libp2p::multi::Multiaddress::create(argv[1]);  // NOLINT
        if (!server_ma_res) {
          log->error("unable to create server multiaddress: {}",
                     server_ma_res.error().message());
          std::exit(EXIT_FAILURE);
        }
        const auto &server_ma = server_ma_res.value();

        auto server_peer_id_str = server_ma.getPeerId();
        if (!server_peer_id_str) {
          log->error("unable to get peer id");
          std::exit(EXIT_FAILURE);
        }

        auto server_peer_id_res =
            libp2p::peer::PeerId::fromBase58(*server_peer_id_str);
        if (!server_peer_id_res) {
          log->error("Unable to decode peer id from base 58: {}",
                     server_peer_id_res.error().message());
          std::exit(EXIT_FAILURE);
        }

        const auto &server_peer_id = server_peer_id_res.value();

        auto peer_info = libp2p::peer::PeerInfo{server_peer_id, {server_ma}};

        // create Host object and open a stream through it
        host->newStream(
            peer_info, {echo.getProtocolId()},
            [log, &echo, &message, sch](auto &&stream_res) {
              if (!stream_res) {
                log->error("Cannot connect to server: {}",
                           stream_res.error().message());
                std::exit(EXIT_FAILURE);
              }

              auto stream_p = std::move(stream_res.value().stream);

              auto echo_client = echo.createClient(stream_p);

              if (message.size() < 120) {
                log->info("SENDING {}", message);
              } else {
                log->info("SENDING {} bytes", message.size());
              }

              sch->schedule(
                  [log, message, stream = std::move(stream_p), echo_client] {
                    echo_client->sendAnd(
                        message,
                        [log,
                         stream = std::move(stream)](auto &&response_result) {
                          if (response_result.has_error()) {
                            log->info("Error happened: {}",
                                      response_result.error().message());
                            stream->close(
                                [log](auto &&) { std::exit(EXIT_SUCCESS); });
                            return;
                          }
                          auto &resp = response_result.value();
                          if (resp.size() < 120) {
                            log->info("RESPONSE {}", resp);
                          } else {
                            log->info("RESPONSE size={}", resp.size());
                          }
                          stream->close(
                              [](auto &&) { std::exit(EXIT_SUCCESS); });
                        });
                  },
                  std::chrono::milliseconds(1000));
            });
      });

  // run the IO context
  context->run_for(run_duration);
}
