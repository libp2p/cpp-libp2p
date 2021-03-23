/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/echo.hpp"

#include <gtest/gtest.h>
#include "mock/libp2p/connection/stream_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace libp2p;
using namespace protocol;
using ::testing::_;
using ::testing::Return;
using std::string_literals::operator""s;

// set what we read
ACTION_P(SetReadMsg, msg) {
  std::copy(msg.begin(), msg.end(), arg0.begin());
  arg2(msg.size());
}

// assert that we write the same as we read
ACTION_P(WriteMsgAssertEqual, msg) {
  std::string sub;

  if (*arg0.begin() == 0) {
    // EOF
    return;
  }

  auto begin = arg0.begin();
  auto end = arg0.begin();

  std::advance(end, msg.size());
  std::copy(begin, end, std::back_inserter(sub));
  EXPECT_EQ(sub, msg);

  arg2(msg.size());
}

/**
 * @given Stream
 * @when server reads string "hello" from Stream
 * @then server writes back the same string
 */
TEST(EchoTest, Server) {
  testutil::prepareLoggers();

  Echo echo;
  auto stream = std::make_shared<connection::StreamMock>();
  auto msg = "hello"s;

  EXPECT_CALL(*stream, isClosedForRead())
      .WillOnce(Return(false))
      .WillOnce(Return(true));

  EXPECT_CALL(*stream, isClosedForWrite()).WillOnce(Return(false));

  EXPECT_CALL(*stream, close(_))
      .WillOnce(Arg0CallbackWithArg(outcome::success()));

  EXPECT_CALL(*stream, readSome(_, _, _)).WillOnce(SetReadMsg(msg));
  EXPECT_CALL(*stream, write(_, _, _)).WillOnce(WriteMsgAssertEqual(msg));

  echo.handle(stream);
}

/**
 * @given Stream
 * @when client writes string "hello" to the Stream
 * @then client reads back the same string
 */
TEST(EchoTest, DISABLED_Client) {
  Echo echo;
  auto stream = std::make_shared<connection::StreamMock>();
  auto msg = "hello"s;

  EXPECT_CALL(*stream, isClosedForWrite()).WillOnce(Return(false));

  EXPECT_CALL(*stream, write(_, _, _)).WillOnce(WriteMsgAssertEqual(msg));
  EXPECT_CALL(*stream, readSome(_, _, _))
      .WillOnce(WriteMsgAssertEqual(msg));

  bool executed = false;

  auto client = echo.createClient(stream);
  client->sendAnd(msg, [&](outcome::result<std::string> rm) {
    EXPECT_OUTCOME_TRUE(m, rm);
    ASSERT_EQ(m, msg);
    executed = true;
  });

  ASSERT_TRUE(executed);
}
