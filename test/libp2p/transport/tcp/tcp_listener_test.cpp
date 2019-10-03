/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <libp2p/common/literals.hpp>
#include "libp2p/transport/tcp/tcp_listener.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/outcome.hpp"

#include "mock/libp2p/transport/upgrader_mock.hpp"

using namespace libp2p;
using namespace transport;
using namespace connection;
using namespace common;
using std::chrono_literals::operator""ms;

using ::testing::_;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::MockFunction;
using ::testing::StrictMock;

struct TcpListenerTest : public ::testing::Test {
  using CapConnResult = outcome::result<std::shared_ptr<CapableConnection>>;

  std::shared_ptr<boost::asio::io_context> context =
      std::make_shared<boost::asio::io_context>();

  std::shared_ptr<StrictMock<UpgraderMock>> upgrader =
      std::make_shared<StrictMock<UpgraderMock>>();

  StrictMock<MockFunction<void(CapConnResult)>> cb;

  std::shared_ptr<TcpListener> listener;

  multi::Multiaddress ma = "/ip4/127.0.0.1/tcp/40005"_multiaddr;

  void SetUp() override {
    listener = std::make_shared<TcpListener>(
        *context, upgrader,
        [this](auto &&r) { cb.Call(std::forward<decltype(r)>(r)); });
  }
};

/**
 * @given listener
 * @when listen, close, listen, close
 * @then no error happens
 */
TEST_F(TcpListenerTest, ListenCloseListen) {
  EXPECT_CALL(cb, Call(_)).WillRepeatedly(Invoke([](CapConnResult c) {
    if (!c) {
      ASSERT_EQ(c.error().value(), (int)std::errc::operation_canceled);
    } else {
      ADD_FAILURE();
    }
  }));

  EXPECT_OUTCOME_TRUE_1(listener->listen(ma));
  ASSERT_FALSE(listener->isClosed());
  EXPECT_OUTCOME_TRUE_1(listener->close());
  ASSERT_TRUE(listener->isClosed());

  EXPECT_OUTCOME_TRUE_1(listener->listen(ma));
  ASSERT_FALSE(listener->isClosed());
  EXPECT_OUTCOME_TRUE_1(listener->close());
  ASSERT_TRUE(listener->isClosed());

  context->run_for(50ms);
}

/**
 * @give listener
 * @when double close
 * @then no error received
 */
TEST_F(TcpListenerTest, DoubleClose) {
  EXPECT_CALL(cb, Call(_)).WillOnce(Invoke([](CapConnResult c) {
    if (!c) {
      ASSERT_EQ(c.error().value(), (int)std::errc::operation_canceled);
    }
  }));

  EXPECT_OUTCOME_TRUE_1(listener->listen(ma));
  ASSERT_FALSE(listener->isClosed());
  EXPECT_OUTCOME_TRUE_1(listener->close());
  EXPECT_OUTCOME_TRUE_1(listener->close());
  ASSERT_TRUE(listener->isClosed());
  context->run_for(50ms);
}
