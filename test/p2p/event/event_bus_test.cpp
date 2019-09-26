/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/event/bus.hpp"

#include <gtest/gtest.h>

using namespace libp2p::event;

class EventBusTest : public testing::Test {
 public:
  struct Event1 {};
  struct Event2 {};

  using Event1Channel = channel_decl<Event1, int>;
  using Event2Channel = channel_decl<Event2, std::string>;

  Bus bus_;
};

/**
 * @given event bus
 * @when getting two events channels from that bus @and subscribing to their
 * events @and publishing the events
 * @then everything goes the intended way
 */
TEST_F(EventBusTest, SubscribePublish) {
  auto &event1_channel = bus_.getChannel<Event1Channel>();
  auto &event2_channel = bus_.getChannel<Event2Channel>();

  int expected_int = 2, int1, int2;
  auto h1 = event1_channel.subscribe([&int1](auto &&n) { int1 = n; });
  auto h2 = event1_channel.subscribe([&int2](auto &&n) { int2 = n; });

  std::string expected_str = "foo", str;
  auto h3 = event2_channel.subscribe(
      [&str](auto &&received_str) { str = received_str; });

  event1_channel.publish(expected_int);
  event2_channel.publish(expected_str);

  h1.unsubscribe();
  h2.unsubscribe();
  h3.unsubscribe();

  ASSERT_EQ(int1, expected_int);
  ASSERT_EQ(int2, expected_int);
  ASSERT_EQ(str, expected_str);
}
