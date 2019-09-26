/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamuxed_connection.hpp"

#include <gtest/gtest.h>
#include "common/buffer.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/security/plaintext.hpp"
#include "libp2p/transport/tcp.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/transport/upgrader_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::connection;
using namespace libp2p::transport;
using namespace kagome::common;
using namespace libp2p::multi;
using namespace libp2p::basic;
using namespace libp2p::security;
using namespace libp2p::muxer;

using testing::_;

using std::chrono_literals::operator""ms;

const Buffer kPingBytes = Buffer{}.put("PING");
const Buffer kPongBytes = Buffer{}.put("PONG");

struct ServerStream : std::enable_shared_from_this<ServerStream> {
  explicit ServerStream(std::shared_ptr<Stream> s)
      : stream{std::move(s)}, read_buffer(kPingBytes.size(), 0) {}

  std::shared_ptr<Stream> stream;
  Buffer read_buffer;

  void doRead() {
    if (stream->isClosedForRead()) {
      return;
    }
    stream->read(read_buffer, read_buffer.size(),
                 [self = shared_from_this()](auto &&res) {
                   ASSERT_TRUE(res);
                   self->readCompleted();
                 });
  }

  void readCompleted() {
    ASSERT_EQ(read_buffer, kPingBytes) << "expected to received a PING message";
    doWrite();
  }

  void doWrite() {
    if (stream->isClosedForWrite()) {
      return;
    }
    stream->write(kPongBytes, kPongBytes.size(),
                  [self = shared_from_this()](auto &&res) {
                    ASSERT_TRUE(res);
                    self->doRead();
                  });
  }
};

/**
 * @given Yamuxed server, which is setup to write 'PONG' for any received 'PING'
 * message @and Yamuxed client, connected to that server
 * @when the client sets up a listener on that server @and writes 'PING'
 * @then the 'PONG' message is received by the client
 */
TEST(YamuxAcceptanceTest, PingPong) {
  auto ma = "/ip4/127.0.0.1/tcp/40009"_multiaddr;
  auto stream_read = false, stream_wrote = false;
  auto context = std::make_shared<boost::asio::io_context>(1);

  auto upgrader = std::make_shared<UpgraderMock>();
  EXPECT_CALL(*upgrader, upgradeToSecureInbound(_, _))
      .WillRepeatedly(
          UpgradeToSecureInbound([](std::shared_ptr<RawConnection> raw)
                                     -> std::shared_ptr<SecureConnection> {
            return std::make_shared<CapableConnBasedOnRawConnMock>(raw);
          }));
  EXPECT_CALL(*upgrader, upgradeToSecureOutbound(_, _, _))
      .WillRepeatedly(
          UpgradeToSecureOutbound([](std::shared_ptr<RawConnection> raw)
                                      -> std::shared_ptr<SecureConnection> {
            return std::make_shared<CapableConnBasedOnRawConnMock>(raw);
          }));
  EXPECT_CALL(*upgrader, upgradeToMuxed(_, _))
      .WillRepeatedly(UpgradeToMuxed([](std::shared_ptr<SecureConnection> sec)
                                         -> std::shared_ptr<CapableConnection> {
        return std::make_shared<YamuxedConnection>(sec);
      }));

  auto transport = std::make_shared<TcpTransport>(context, upgrader);
  ASSERT_TRUE(transport) << "cannot create transport";

  auto transport_listener = transport->createListener([](auto &&conn_res) {
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    conn->onStream([](auto &&stream) {
      // wrap each received stream into a server structure and start
      // reading
      ASSERT_TRUE(stream);
      auto server = std::make_shared<ServerStream>(
          std::forward<decltype(stream)>(stream));
      server->doRead();
    });

    conn->start();
  });

  ASSERT_TRUE(transport_listener->listen(ma)) << "is port 40009 busy?";

  transport->dial(testutil::randomPeerId(), ma, [&](auto &&conn_res) {
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    conn->start();

    conn->newStream([&](auto &&stream_res) mutable {
      EXPECT_OUTCOME_TRUE(stream, stream_res)
      auto stream_read_buffer = std::make_shared<Buffer>(kPongBytes.size(), 0);

      // proof our streams have parallelism: set up both read and write on the
      // stream and make sure they are successfully executed
      stream->read(*stream_read_buffer, stream_read_buffer->size(),
                   [&, stream_read_buffer](auto &&res) {
                     ASSERT_EQ(*stream_read_buffer, kPongBytes);
                     stream_read = true;
                   });

      stream->write(kPingBytes, kPingBytes.size(), [&stream_wrote](auto &&res) {
        ASSERT_TRUE(res);
        stream_wrote = true;
      });
    });
  });

  // let the streams make their jobs
  context->run_for(100ms);

  EXPECT_TRUE(stream_read);
  EXPECT_TRUE(stream_wrote);
}
