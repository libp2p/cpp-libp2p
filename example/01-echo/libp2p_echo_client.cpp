/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <memory>

#include <libp2p/common/literals.hpp>
#include <libp2p/common/logger.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/protocol/echo.hpp>

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
      jumbo_message.reserve(sz + message.size());
      for (size_t i = 0, count = sz / message.size(); i < count; ++i) {
        jumbo_message.append(message);
      }
      jumbo_message.resize(sz);
      message.swap(jumbo_message);
      run_duration = std::chrono::seconds(150);
    }
  }

  if (std::getenv("TRACE_DEBUG") != nullptr) {
    spdlog::set_level(spdlog::level::trace);
    run_duration = std::chrono::seconds(86400);
  } else {
    spdlog::set_level(spdlog::level::err);
  }

  if (argc < 2) {
    std::cerr << "please, provide an address of the server\n";
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
  context->post([host{std::move(host)}, &echo, &message, argv] {  // NOLINT
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
        peer_info, echo.getProtocolId(), [&echo, &message](auto &&stream_res) {
          if (!stream_res) {
            std::cerr << "Cannot connect to server: "
                      << stream_res.error().message() << std::endl;
            std::exit(EXIT_FAILURE);
          }

          auto stream_p = std::move(stream_res.value());

          auto echo_client = echo.createClient(stream_p);

          if (message.size() < 120) {
            std::cout << "SENDING " << message << "\n";
          } else {
            std::cout << "SENDING " << message.size() << " bytes" << std::endl;
          }
          echo_client->sendAnd(
              message, [stream = std::move(stream_p)](auto &&response_result) {
                auto &resp = response_result.value();
                if (resp.size() < 120) {
                  std::cout << "RESPONSE " << resp << std::endl;
                } else {
                  std::cout << "RESPONSE size=" << resp.size() << std::endl;
                }
                stream->close([](auto &&) { std::exit(EXIT_SUCCESS); });
              });
        });
  });

  // run the IO context
  context->run_for(run_duration);
}
