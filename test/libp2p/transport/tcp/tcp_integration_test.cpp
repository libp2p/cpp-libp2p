/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/transport/tcp.hpp>
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/transport/upgrader_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace libp2p::transport;
using namespace libp2p::multi;
using namespace libp2p::common;
using namespace libp2p::connection;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;
using libp2p::common::ByteArray;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace {
  std::shared_ptr<CapableConnection> expectConnectionValid(
      libp2p::outcome::result<std::shared_ptr<CapableConnection>> rconn) {
    EXPECT_TRUE(rconn) << rconn.error();
    auto conn = rconn.value();

    EXPECT_OUTCOME_TRUE(mar, conn->remoteMultiaddr())
    EXPECT_OUTCOME_TRUE(mal, conn->localMultiaddr())
    std::ostringstream s;
    s << mar.getStringAddress() << " -> " << mal.getStringAddress();
    std::cout << s.str() << '\n';

    return conn;
  }

  auto makeUpgrader() {
    auto upgrader = std::make_shared<NiceMock<UpgraderMock>>();
    ON_CALL(*upgrader, upgradeToSecureOutbound(_, _, _))
        .WillByDefault(UpgradeToSecureOutbound([](auto &&raw) {
          std::shared_ptr<SecureConnection> sec =
              std::make_shared<CapableConnBasedOnRawConnMock>(raw);
          return sec;
        }));
    ON_CALL(*upgrader, upgradeToSecureInbound(_, _))
        .WillByDefault(UpgradeToSecureInbound([](auto &&raw) {
          std::shared_ptr<SecureConnection> sec =
              std::make_shared<CapableConnBasedOnRawConnMock>(raw);
          return sec;
        }));
    ON_CALL(*upgrader, upgradeToMuxed(_, _))
        .WillByDefault(UpgradeToMuxed([](auto &&sec) {
          std::shared_ptr<CapableConnection> cap =
              std::make_shared<CapableConnBasedOnRawConnMock>(sec);
          return cap;
        }));

    return upgrader;
  }
}  // namespace

/**
 * @given two listeners
 * @when bound on the same multiaddress
 * @then get error
 */
TEST(TCP, TwoListenersCantBindOnSamePort) {
  auto context = std::make_shared<boost::asio::io_context>(1);
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto listener1 = transport->createListener([](auto &&c) { EXPECT_TRUE(c); });

  ASSERT_TRUE(listener1);

  auto listener2 = transport->createListener([](auto &&c) { EXPECT_TRUE(c); });
  ASSERT_TRUE(listener2);

  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  std::cout << "listener 1 starting...\n";
  ASSERT_TRUE(listener1->listen(ma));

  std::cout << "listener 2 starting...\n";
  auto r = listener2->listen(ma);
  ASSERT_EQ(r.error().value(), (int)std::errc::address_in_use);

  using std::chrono_literals::operator""ms;
  context->run_for(50ms);
}

/**
 * @given Echo server with single listener
 * @when parallel clients connect and send random message
 * @then each client is expected to receive sent message
 */
TEST(TCP, SingleListenerCanAcceptManyClients) {
  constexpr static int kClients = 2;
  constexpr static int kSize = 1500;
  size_t counter = 0;  // number of answers
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  auto context = std::make_shared<boost::asio::io_context>();
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  using libp2p::connection::RawConnection;
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());

    auto buf = std::make_shared<std::vector<uint8_t>>(kSize, 0);
    conn->readSome(*buf, buf->size(),
                   [&counter, conn, buf, context](auto &&res) {
                     ASSERT_TRUE(res) << res.error().message();

                     conn->write(*buf, buf->size(),
                                 [&counter, conn, buf, context](auto &&res) {
                                   ASSERT_TRUE(res) << res.error().message();
                                   EXPECT_EQ(res.value(), buf->size());
                                   counter++;
                                   if (counter >= kClients){
                                     context->stop();
                                   }
                                 });
                   });
  });

  ASSERT_TRUE(listener);
  ASSERT_TRUE(listener->listen(ma));

  std::vector<std::thread> clients(kClients);
  std::generate(clients.begin(), clients.end(), [&]() {
    return std::thread([&]() {
      auto context = std::make_shared<boost::asio::io_context>();
      auto upgrader = makeUpgrader();
      auto transport =
          std::make_shared<TcpTransport>(context, std::move(upgrader));
      transport->dial(testutil::randomPeerId(), ma, [context](auto &&rconn) {
        auto conn = expectConnectionValid(rconn);

        auto readback = std::make_shared<ByteArray>(kSize, 0);
        auto buf = std::make_shared<ByteArray>(kSize, 0);
        std::generate(buf->begin(), buf->end(), []() {
          return rand();  // NOLINT
        });

        EXPECT_TRUE(conn->isInitiator());

        conn->write(*buf, buf->size(),
                    [conn, readback, buf, context](auto &&res) {
                      ASSERT_TRUE(res) << res.error().message();
                      ASSERT_EQ(res.value(), buf->size());
                      conn->read(*readback, readback->size(),
                                 [conn, readback, buf, context](auto &&res) {
                                   context->stop();
                                   ASSERT_TRUE(res) << res.error().message();
                                   ASSERT_EQ(res.value(), readback->size());
                                   ASSERT_EQ(*buf, *readback);
                                 });
                    });
      });

      context->run_for(400ms);
    });
  });

  context->run_for(500ms);
  std::for_each(clients.begin(), clients.end(),
                [](std::thread &t) { t.join(); });

  ASSERT_EQ(counter, kClients) << "not all clients' requests were handled";
}

/**
 * @given tcp transport
 * @when dial to non-existent server (listener)
 * @then get connection_refused error
 */
TEST(TCP, DialToNoServer) {
  auto context = std::make_shared<boost::asio::io_context>();
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  transport->dial(testutil::randomPeerId(), ma, [](auto &&rc) {
    ASSERT_FALSE(rc);
    ASSERT_EQ(rc.error().value(), (int)std::errc::connection_refused);
  });

  using std::chrono_literals::operator""ms;
  context->run_for(50ms);
}

/**
 * @given server with one active client
 * @when client closes connection
 * @then server gets EOF
 */
TEST(TCP, ClientClosesConnection) {
  auto context = std::make_shared<boost::asio::io_context>(1);
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());

    auto buf = std::make_shared<std::vector<uint8_t>>(100, 0);
    conn->readSome(*buf, buf->size(), [conn, buf](auto &&res) {
      ASSERT_FALSE(res);
      ASSERT_EQ(res.error().value(), (int)boost::asio::error::eof)
          << res.error().message();
    });
  });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(testutil::randomPeerId(), ma, [](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_TRUE(conn->isInitiator());
    EXPECT_TRUE(conn->close());
  });

  context->run_for(50ms);
}

/**
 * @given server with one active client
 * @when server closes active connection
 * @then client gets EOF
 */
TEST(TCP, ServerClosesConnection) {
  auto context = std::make_shared<boost::asio::io_context>(1);
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());
    EXPECT_OUTCOME_TRUE_1(conn->close())
  });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(testutil::randomPeerId(), ma, [](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_TRUE(conn->isInitiator());
    auto buf = std::make_shared<std::vector<uint8_t>>(100, 0);
    conn->readSome(*buf, buf->size(), [conn, buf](auto &&res) {
      ASSERT_FALSE(res);
      ASSERT_EQ(res.error().value(), (int)boost::asio::error::eof)
          << res.error().message();
    });
  });

  context->run_for(50ms);
}

/**
 * @given single thread, single transport on a single default executor
 * @when create server @and dial to this server
 * @then connection successfully established
 */
TEST(TCP, OneTransportServerHandlesManyClients) {
  constexpr int kSize = 1500;
  size_t counter = 0;  // number of answers

  auto context = std::make_shared<boost::asio::io_context>(1);
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());

    auto buf = std::make_shared<std::vector<uint8_t>>(kSize, 0);
    conn->readSome(*buf, kSize, [kSize, &counter, conn, buf](auto &&res) {
      ASSERT_TRUE(res) << res.error().message();

      conn->write(*buf, kSize, [&counter, buf, conn](auto &&res) {
        ASSERT_TRUE(res) << res.error().message();
        EXPECT_EQ(res.value(), buf->size());
        counter++;
      });
    });
  });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(
      testutil::randomPeerId(),  // ignore arg
      ma, [kSize](auto &&rconn) {
        auto conn = expectConnectionValid(rconn);

        auto readback = std::make_shared<ByteArray>(kSize, 0);
        auto buf = std::make_shared<ByteArray>(kSize, 0);
        std::generate(buf->begin(), buf->end(), []() {
          return rand();  // NOLINT
        });

        EXPECT_TRUE(conn->isInitiator());

        conn->write(*buf, kSize, [conn, kSize, readback, buf](auto &&res) {
          ASSERT_TRUE(res) << res.error().message();
          ASSERT_EQ(res.value(), buf->size());
          conn->read(*readback, kSize, [conn, readback, buf](auto &&res) {
            ASSERT_TRUE(res) << res.error().message();
            ASSERT_EQ(res.value(), readback->size());
            ASSERT_EQ(*buf, *readback);
          });
        });
      });

  context->run_for(100ms);

  ASSERT_EQ(counter, 1);
}

int main(int argc, char *argv[]) {
  if (std::getenv("TRACE_DEBUG") != nullptr) {
    testutil::prepareLoggers(soralog::Level::TRACE);
  } else {
    testutil::prepareLoggers(soralog::Level::ERROR);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
