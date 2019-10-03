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
  host->setProtocolHandler(
      echo.getProtocolId(),
      [&echo](std::shared_ptr<libp2p::connection::Stream> received_stream) {
        echo.handle(std::move(received_stream));
      });

  // launch a Listener part of the Host
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  context->post([host{std::move(host)}] {
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
              << "\nPeer id: " << host->getPeerInfo().id.toBase58() << "\n";
  });

  // run the IO context
  context->run_for(std::chrono::seconds(5));
}
