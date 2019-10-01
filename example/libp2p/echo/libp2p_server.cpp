/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <memory>

#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/protocol/echo.hpp>
#include <libp2p/security/plaintext.hpp>

int main() {
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;

  // this keypair generates a PeerId
  // "12D3KooWLs7RC93EGXZzn9YdKyZYYx3f9UjTLYNX1reThpCkFb83"
  KeyPair keypair{
      PublicKey{
          {Key::Type::Ed25519,
           {0xa4, 0x24, 0x9e, 0xa6, 0xd6, 0x2b, 0xdd, 0x8b, 0xcc, 0xf6, 0x22,
            0x57, 0xac, 0x48, 0x99, 0xff, 0x28, 0x47, 0x96, 0x32, 0x28, 0xb3,
            0x88, 0xfd, 0xa2, 0x88, 0xdb, 0x5d, 0x64, 0xe5, 0x17, 0xe0}}},
      PrivateKey{
          {Key::Type::Ed25519,
           {0x4a, 0x93, 0x61, 0xc5, 0x25, 0x84, 0x0f, 0x70, 0x86, 0xb8, 0x93,
            0xd5, 0x84, 0xeb, 0xbe, 0x47, 0x5b, 0x4e, 0xc7, 0x06, 0x99, 0x51,
            0xd2, 0xe8, 0x97, 0xe8, 0xbc, 0xeb, 0x0a, 0x3f, 0x35, 0xce}}}};

  // create a default Host via an injector, overriding a random-generated
  // keypair with ours
  auto injector =
      libp2p::injector::makeHostInjector(libp2p::injector::useKeyPair(keypair));
  auto host = injector.create<std::shared_ptr<libp2p::Host>>();

  // set a handler for Echo protocol
  std::shared_ptr<libp2p::connection::Stream> stream;
  libp2p::protocol::Echo echo;
  host->setProtocolHandler(
      echo.getProtocolId(),
      [&echo, &stream](
          std::shared_ptr<libp2p::connection::Stream> received_stream) mutable {
        stream = received_stream;
        echo.handle(std::move(received_stream));
      });

  // launch a Listener part of the Host
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  context->post([host{std::move(host)}] {
    auto ma =
        libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/40010").value();
    auto res = host->listen(ma);
    if (!res) {
      std::cerr << res.error().message();
    }

    host->start();
    std::cout << "Server started. Peer id: ";
    std::cout << host->getPeerInfo().id.toBase58() << std::endl;
  });

  // run the IO context
  context->run_for(std::chrono::seconds(5));

  // close the stream after done, as Go implementation relies on it
  stream->close([](auto &&) {});
}
