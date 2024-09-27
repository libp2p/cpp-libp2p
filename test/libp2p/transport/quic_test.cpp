/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/basic/read.hpp>
#include <libp2p/basic/write.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <qtils/bytestr.hpp>

#include "testutil/prepare_loggers.hpp"

using boost::asio::io_context;
using libp2p::Host;
using libp2p::Multiaddress;
using libp2p::StreamAndProtocol;
using libp2p::StreamAndProtocolOrError;
using libp2p::connection::Stream;
using qtils::byte2str;
using qtils::str2byte;

auto makeInjector(std::shared_ptr<io_context> io) {
  return libp2p::injector::makeHostInjector<
      boost::di::extension::shared_config>(
      boost::di::bind<io_context>().to(io));
}
using Injector = decltype(makeInjector(nullptr));

struct Peer {
  Peer(std::shared_ptr<io_context> io) : injector{makeInjector(io)} {
    inject(io);
    inject(host);
  }
  template <typename T>
  void inject(T &t) {
    t = injector.create<T>();
  }

  Injector injector;
  std::shared_ptr<Host> host;
  std::shared_ptr<Stream> stream;
};

/**
 * Test quic transport, both client and server.
 *
 * Client connects to server and opens stream.
 * Client writes request and reads response.
 * Server reads request and writes response.
 */
TEST(Quic, Test) {
  testutil::prepareLoggers();
  std::string protocol = "/test";
  std::string_view req{"request"}, res{"response"};
  auto io = std::make_shared<io_context>();
  auto run = [&] {
    io->restart();
    io->run_for(std::chrono::seconds{1});
  };
  size_t wait_count = 0;
  auto wait_done = [&] {
    if (--wait_count == 0) {
      io->stop();
    }
  };
  Peer client{io}, server{io};
  auto addr =
      Multiaddress::create(fmt::format("/ip4/127.0.0.1/udp/{}/quic-v1", 10001))
          .value();
  server.host->listen(addr).value();
  server.host->start();

  wait_count = 2;
  server.host->setProtocolHandler({protocol}, [&](StreamAndProtocol r) {
    server.stream = r.stream;
    wait_done();
  });
  client.host->newStream(
      server.host->getPeerInfo(), {protocol}, [&](StreamAndProtocolOrError r) {
        client.stream = r.value().stream;
        wait_done();
      });
  run();
  EXPECT_TRUE(client.stream);
  EXPECT_TRUE(server.stream);

#define RW_CB                    \
  [&](outcome::result<void> r) { \
    r.value();                   \
    wait_done();                 \
  }

  wait_count = 2;
  qtils::Bytes req_out(req.size());
  libp2p::write(client.stream, str2byte(req), RW_CB);
  libp2p::read(server.stream, req_out, RW_CB);
  run();
  EXPECT_EQ(byte2str(req_out), req);

  wait_count = 2;
  qtils::Bytes res_out(res.size());
  libp2p::read(client.stream, res_out, RW_CB);
  libp2p::write(server.stream, str2byte(res), RW_CB);
  run();
  EXPECT_EQ(byte2str(res_out), res);
}
