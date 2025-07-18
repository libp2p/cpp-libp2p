/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <libp2p/basic/read.hpp>
#include <libp2p/basic/write.hpp>
#include <libp2p/common/literals.hpp>
#include <libp2p/transport/tcp.hpp>
#include <memory>
#include <qtils/test/outcome.hpp>
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/transport/upgrader_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace libp2p::transport;
using namespace libp2p::multi;
using namespace libp2p::common;
using namespace libp2p::connection;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;
using libp2p::Bytes;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

libp2p::muxer::MuxedConnectionConfig mux_config;

namespace {
  std::shared_ptr<CapableConnection> expectConnectionValid(
      outcome::result<std::shared_ptr<CapableConnection>> rconn) {
    EXPECT_OUTCOME_SUCCESS(rconn);
    auto conn = rconn.value();

    EXPECT_OUTCOME_SUCCESS(mar, conn->remoteMultiaddr());
    EXPECT_OUTCOME_SUCCESS(mal, conn->localMultiaddr());
    std::ostringstream s;
    s << mar.value().getStringAddress() << " -> "
      << mal.value().getStringAddress();
    std::cout << s.str() << '\n';

    return conn;
  }

  auto makeUpgrader() {
    auto upgrader = std::make_shared<NiceMock<UpgraderMock>>();
    ON_CALL(*upgrader, upgradeLayersOutbound(_, _, _, _))
        .WillByDefault(UpgradeLayersOutbound([](auto &&raw) {
          std::shared_ptr<LayerConnection> layer_connection =
              std::make_shared<CapableConnBasedOnLayerConnMock>(raw);
          return layer_connection;
        }));
    ON_CALL(*upgrader, upgradeLayersInbound(_, _, _))
        .WillByDefault(UpgradeLayersInbound([](auto &&raw) {
          std::shared_ptr<LayerConnection> layer_connection =
              std::make_shared<CapableConnBasedOnLayerConnMock>(raw);
          return layer_connection;
        }));
    ON_CALL(*upgrader, upgradeToSecureOutbound(_, _, _))
        .WillByDefault(UpgradeToSecureOutbound([](auto &&layer_connection) {
          std::shared_ptr<SecureConnection> secure_connection =
              std::make_shared<CapableConnBasedOnLayerConnMock>(
                  layer_connection);
          return secure_connection;
        }));
    ON_CALL(*upgrader, upgradeToSecureInbound(_, _))
        .WillByDefault(UpgradeToSecureInbound([](auto &&layer_connection) {
          std::shared_ptr<SecureConnection> secure_connection =
              std::make_shared<CapableConnBasedOnLayerConnMock>(
                  layer_connection);
          return secure_connection;
        }));
    ON_CALL(*upgrader, upgradeToMuxed(_, _))
        .WillByDefault(UpgradeToMuxed([](auto &&sec) {
          std::shared_ptr<CapableConnection> cap =
              std::make_shared<CapableConnBasedOnLayerConnMock>(sec);
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
  auto transport =
      std::make_shared<TcpTransport>(context, mux_config, std::move(upgrader));
  auto listener1 = transport->createListener([](auto &&c) { EXPECT_TRUE(c); });

  ASSERT_TRUE(listener1);

  auto listener2 = transport->createListener([](auto &&c) { EXPECT_TRUE(c); });
  ASSERT_TRUE(listener2);

  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  std::cout << "listener 1 starting...\n";
  ASSERT_TRUE(listener1->listen(ma));

  std::cout << "listener 2 starting...\n";
  auto r = listener2->listen(ma);
  if (!r) {
    ASSERT_EQ(r.error().value(), (int)boost::asio::error::address_in_use);
  } else {
    ADD_FAILURE();
  }

  using std::chrono_literals::operator""ms;
  context->run_for(50ms);
}

/**
 * @given Echo server with single listener
 * @when parallel clients connect and send random message
 * @then each client is expected to receive sent message
 */
TEST(TCP, SingleListenerCanAcceptManyClients) {
  static constexpr int kClients = 2;
  static constexpr int kSize = 1500;
  size_t counter = 0;  // number of answers
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  auto context = std::make_shared<boost::asio::io_context>();
  auto upgrader = makeUpgrader();
  auto transport =
      std::make_shared<TcpTransport>(context, mux_config, std::move(upgrader));
  using libp2p::connection::RawConnection;
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());

    auto buf = std::make_shared<std::vector<uint8_t>>(kSize, 0);
    conn->readSome(*buf, [&counter, conn, buf, context](auto &&res) {
      ASSERT_OUTCOME_SUCCESS(res);

      libp2p::write(conn,
                    *buf,
                    [&counter, conn, buf, context](outcome::result<void> res) {
                      ASSERT_OUTCOME_SUCCESS(res);
                      counter++;
                      if (counter >= kClients) {
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
      auto transport = std::make_shared<TcpTransport>(
          context, mux_config, std::move(upgrader));
      transport->dial(testutil::randomPeerId(), ma, [context](auto &&rconn) {
        auto conn = expectConnectionValid(rconn);

        auto readback = std::make_shared<Bytes>(kSize, 0);
        auto buf = std::make_shared<Bytes>(kSize, 0);
        std::generate(buf->begin(), buf->end(), []() {
          return rand();  // NOLINT
        });

        EXPECT_TRUE(conn->isInitiator());

        libp2p::write(
            conn,
            *buf,
            [conn, readback, buf, context](outcome::result<void> res) {
              ASSERT_OUTCOME_SUCCESS(res);
              libp2p::read(
                  conn,
                  *readback,
                  [conn, readback, buf, context](outcome::result<void> res) {
                    context->stop();
                    ASSERT_OUTCOME_SUCCESS(res);
                    ASSERT_EQ(*buf, *readback);
                  });
            });
      });

      context->run_for(400ms);
    });
  });

  context->run_for(500ms);
  for (auto &t : clients) {
    t.join();
  }

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
  auto transport =
      std::make_shared<TcpTransport>(context, mux_config, std::move(upgrader));
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  transport->dial(testutil::randomPeerId(), ma, [](auto &&rc) {
    ASSERT_OUTCOME_ERROR(rc, boost::asio::error::connection_refused);
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
  auto transport =
      std::make_shared<TcpTransport>(context, mux_config, std::move(upgrader));
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());

    auto buf = std::make_shared<std::vector<uint8_t>>(100, 0);
    conn->readSome(*buf, [conn, buf](auto &&res) {
      ASSERT_OUTCOME_ERROR(res, boost::asio::error::eof);
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
  auto transport =
      std::make_shared<TcpTransport>(context, mux_config, std::move(upgrader));
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());
    ASSERT_OUTCOME_SUCCESS(conn->close());
  });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(testutil::randomPeerId(), ma, [](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_TRUE(conn->isInitiator());
    auto buf = std::make_shared<std::vector<uint8_t>>(100, 0);
    conn->readSome(*buf, [conn, buf](auto &&res) {
      ASSERT_OUTCOME_ERROR(res, boost::asio::error::eof);
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
  auto transport =
      std::make_shared<TcpTransport>(context, mux_config, std::move(upgrader));
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());

    auto buf = std::make_shared<std::vector<uint8_t>>(kSize, 0);
    conn->readSome(*buf, [&counter, conn, buf](auto &&res) {
      ASSERT_OUTCOME_SUCCESS(res);

      libp2p::write(
          conn, *buf, [&counter, buf, conn](outcome::result<void> res) {
            ASSERT_OUTCOME_SUCCESS(res);
            counter++;
          });
    });
  });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(
      testutil::randomPeerId(),  // ignore arg
      ma,
      [kSize](auto &&rconn) {
        auto conn = expectConnectionValid(rconn);

        auto readback = std::make_shared<Bytes>(kSize, 0);
        auto buf = std::make_shared<Bytes>(kSize, 0);
        std::generate(buf->begin(), buf->end(), []() {
          return rand();  // NOLINT
        });

        EXPECT_TRUE(conn->isInitiator());

        libp2p::write(
            conn, *buf, [conn, readback, buf](outcome::result<void> res) {
              ASSERT_OUTCOME_SUCCESS(res);
              libp2p::read(conn,
                           *readback,
                           [conn, readback, buf](outcome::result<void> res) {
                             ASSERT_OUTCOME_SUCCESS(res);
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
