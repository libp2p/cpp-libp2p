/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <memory>

#include <libp2p/common/literals.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/protocol/echo.hpp>

int main() {
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::common::operator""_unhex;

  // this keypair generates a PeerId
  // "12D3KooWLs7RC93EGXZzn9YdKyZYYx3f9UjTLYNX1reThpCkFb83"
  KeyPair keypair{PublicKey{{Key::Type::Ed25519,
                             "a4249ea6d62bdd8bccf62257ac4899ff284796"
                             "3228b388fda288db5d64e517e0"_unhex}},
                  PrivateKey{{Key::Type::Ed25519,
                              "4a9361c525840f7086b893d584ebbe475b4ec"
                              "7069951d2e897e8bceb0a3f35ce"_unhex}}};

  // create a default Host via an injector, overriding a random-generated
  // keypair with ours
  auto injector =
      libp2p::injector::makeHostInjector(libp2p::injector::useKeyPair(keypair));
  auto host = injector.create<std::shared_ptr<libp2p::Host>>();

  // set a handler for Echo protocol
  libp2p::protocol::Echo echo{libp2p::protocol::EchoConfig{1}};
  // host->setProtocolHandler(
  //     echo.getProtocolId(),
  //     [&echo](std::shared_ptr<libp2p::connection::Stream> received_stream) {
  //       echo.handle(std::move(received_stream));
  //     });

  // launch a Listener part of the Host
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  context->post([host{std::move(host)}, &echo] {
    // auto ma =
    //     libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/40010").value();
    // auto listen_res = host->listen(ma);
    // if (!listen_res) {
    //   std::cerr << "host cannot listen the given multiaddress: "
    //             << listen_res.error().message() << "\n";
    //   std::exit(EXIT_FAILURE);
    // }

    // // host->start();
    // std::cout << "Server started\nListening on: " << ma.getStringAddress()
    //           << "\nPeer id: " << host->getPeerInfo().id.toBase58() << "\n";

    auto server_ma_res = libp2p::multi::Multiaddress::create(
        "/ip4/127.0.0.1/tcp/40011/ipfs/"
        "QmXPz2SzFTpzo5VNzeWeuBZTuEymsnNZrkEXyYxG8fiCbQ");
    // "/ip4/127.0.0.1/tcp/40010/ipfs/"
    // "12D3KooWLs7RC93EGXZzn9YdKyZYYx3f9UjTLYNX1reThpCkFb83");
    if (!server_ma_res) {
      std::cerr << "Unable to create server multiaddress: "
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
    host->newStream(
        peer_info, echo.getProtocolId(), [&echo](auto &&stream_res) {
          if (!stream_res) {
            std::cerr << "Cannot connect to server: "
                      << stream_res.error().message() << std::endl;
            std::exit(EXIT_FAILURE);
          }
          auto stream_p = std::move(stream_res.value());

          auto echo_client = echo.createClient(stream_p);
          echo_client->sendAnd(
              "PAVEL DID THE WRONG CHAT",
              [stream = std::move(stream_p)](auto &&response_result) {
                std::cout << "RESPONSE " << response_result.value()
                          << std::endl;
                stream->close([](auto &&) { std::exit(EXIT_SUCCESS); });
              });
          // echo.handle(std::forward<decltype(stream_res)>(stream_res));
        });
  });

  // run the IO context
  context->run_for(std::chrono::seconds(5));
}
