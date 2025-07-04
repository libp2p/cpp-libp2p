/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/protocol/echo.hpp>
#include <qtils/bytestr.hpp>
#include <qtils/test/outcome.hpp>
#include "mock/libp2p/connection/stream_mock.hpp"
#include "testutil/expect_read.hpp"
#include "testutil/expect_write.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace libp2p;
using namespace protocol;
using ::testing::_;
using ::testing::Return;
using std::string_literals::operator""s;

const auto msg = "hello"s;
const auto msg_bytes = qtils::str2byte(msg);

/**
 * @given Stream
 * @when server reads string "hello" from Stream
 * @then server writes back the same string
 */
TEST(EchoTest, Server) {
  testutil::prepareLoggers();

  Echo echo;
  auto stream = std::make_shared<connection::StreamMock>();

  EXPECT_CALL(*stream, isClosedForRead())
      .WillOnce(Return(false))
      .WillOnce(Return(true));

  EXPECT_CALL(*stream, isClosedForWrite()).WillOnce(Return(false));

  EXPECT_CALL(*stream, close(_))
      .WillOnce(Arg0CallbackWithArg(outcome::success()));

  EXPECT_CALL_READ(*stream).WILL_READ(msg_bytes);
  EXPECT_CALL_WRITE(*stream).WILL_WRITE(msg_bytes);

  echo.handle(StreamAndProtocol{stream, {}});
}

/**
 * @given Stream
 * @when client writes string "hello" to the Stream
 * @then client reads back the same string
 */
TEST(EchoTest, Client) {
  Echo echo;
  auto stream = std::make_shared<connection::StreamMock>();

  EXPECT_CALL(*stream, isClosedForWrite()).WillOnce(Return(false));

  EXPECT_CALL_READ(*stream).WILL_READ(msg_bytes).WILL_READ_ERROR();
  EXPECT_CALL_WRITE(*stream).WILL_WRITE(msg_bytes);

  bool executed = false;

  auto client = echo.createClient(stream);
  client->sendAnd(msg, [&](outcome::result<std::string> rm) {
    ASSERT_OUTCOME_SUCCESS(m, rm);
    ASSERT_EQ(m, msg);
    executed = true;
  });

  ASSERT_TRUE(executed);
}
