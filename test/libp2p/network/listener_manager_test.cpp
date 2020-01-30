/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/network/impl/listener_manager_impl.hpp"

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/network/connection_manager_mock.hpp"
#include "mock/libp2p/network/router_mock.hpp"
#include "mock/libp2p/network/transport_manager_mock.hpp"
#include "mock/libp2p/protocol/base_protocol_mock.hpp"
#include "mock/libp2p/protocol_muxer/protocol_muxer_mock.hpp"
#include "mock/libp2p/transport/transport_listener_mock.hpp"
#include "mock/libp2p/transport/transport_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;
using namespace network;
using namespace protocol_muxer;
using namespace transport;
using namespace protocol;
using namespace connection;
using namespace common;

using ::testing::_;
using ::testing::Eq;
using ::testing::MockFunction;
using ::testing::Return;

struct ListenerManagerTest : public ::testing::Test {
  ~ListenerManagerTest() override = default;

  std::shared_ptr<StreamMock> stream = std::make_shared<StreamMock>();

  std::shared_ptr<BaseProtocolMock> protocol =
      std::make_shared<BaseProtocolMock>();

  std::shared_ptr<TransportListenerMock> transport_listener =
      std::make_shared<TransportListenerMock>();

  std::shared_ptr<TransportMock> transport = std::make_shared<TransportMock>();

  std::shared_ptr<ProtocolMuxerMock> proto_muxer =
      std::make_shared<ProtocolMuxerMock>();

  std::shared_ptr<RouterMock> router = std::make_shared<RouterMock>();

  std::shared_ptr<TransportManagerMock> tmgr =
      std::make_shared<TransportManagerMock>();

  std::shared_ptr<ConnectionManagerMock> cmgr =
      std::make_shared<ConnectionManagerMock>();

  std::shared_ptr<ListenerManager> listener =
      std::make_shared<ListenerManagerImpl>(proto_muxer, router, tmgr, cmgr);
};

/**
 * @given 0 transport listeners
 * @when listen on valid address
 * @then new transport listener is created
 */
TEST_F(ListenerManagerTest, ListenValidAddr) {
  EXPECT_CALL(*tmgr, findBest(_)).Times(2).WillRepeatedly(Return(transport));
  EXPECT_CALL(*transport, createListener(_))
      .WillOnce(Return(transport_listener));

  auto supported = "/ip4/127.0.0.1/tcp/0"_multiaddr;
  EXPECT_OUTCOME_TRUE_1(listener->listen(supported));

  EXPECT_EQ(listener->getListenAddresses(),
            std::vector<multi::Multiaddress>{supported});

  auto random_port_resolved = "/ip4/127.0.0.1/tcp/12345"_multiaddr;
  EXPECT_CALL(*transport_listener, getListenMultiaddr())
      .WillOnce(Return(random_port_resolved));

  auto addrs_resolved = listener->getListenAddressesInterfaces();
  EXPECT_EQ(addrs_resolved,
            std::vector<multi::Multiaddress>{random_port_resolved});

  // do listen on the same addr
  EXPECT_OUTCOME_FALSE_1(listener->listen(supported));
}

/**
 * @given 0 transport listeners
 * @when listen on unsupported addr
 * @then listen fails
 */
TEST_F(ListenerManagerTest, ListenInvalidAddr) {
  EXPECT_CALL(*tmgr, findBest(_)).WillOnce(Return(nullptr));

  auto unsupported = "/ip4/127.0.0.1/udp/0"_multiaddr;
  EXPECT_OUTCOME_FALSE_1(listener->listen(unsupported));

  EXPECT_TRUE(listener->getListenAddresses().empty());

  auto addrs_resolved = listener->getListenAddressesInterfaces();
  EXPECT_TRUE(addrs_resolved.empty());
}

/**
 * @given 1 transport listener
 * @when start then stop the listener
 * @then listener started, then stopped without error
 */
TEST_F(ListenerManagerTest, StartStop) {
  EXPECT_CALL(*tmgr, findBest(_)).WillOnce(Return(transport));
  EXPECT_CALL(*transport, createListener(_))
      .WillOnce(Return(transport_listener));
  EXPECT_CALL(*transport_listener, listen(_))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*transport_listener, close())
      .WillOnce(Return(outcome::success()));

  // given 1 listener
  auto supported = "/ip4/127.0.0.1/tcp/0"_multiaddr;
  EXPECT_OUTCOME_TRUE_1(listener->listen(supported));

  // when start
  ASSERT_NO_FATAL_FAILURE(listener->start());
  ASSERT_TRUE(listener->isStarted());
  ASSERT_NO_FATAL_FAILURE(listener->stop());
  ASSERT_FALSE(listener->isStarted());
}

/**
 * @given listener
 * @when setProtocolHandler is executed
 * @then router successfully binds this protocol
 */
TEST_F(ListenerManagerTest, SetProtocolHandler) {
  using std::string_literals::operator""s;
  auto p = "/test/1.0.0"s;

  MockFunction<void(ListenerManager::StreamResult)> cb;

  EXPECT_CALL(cb, Call(Eq(outcome::success(stream)))).Times(1);

  EXPECT_CALL(*router, setProtocolHandler(Eq(p), _))
      .WillOnce(Arg1CallbackWithArg(stream));

  listener->setProtocolHandler(p, [&](auto &&s) { cb.Call(s); });
}

/**
 * @given listener
 * @when setProtocolHandler is executed
 * @then router successfully binds this protocol
 */
TEST_F(ListenerManagerTest, SetProtocolHandlerWithMatcher) {
  using std::string_literals::operator""s;
  auto p = "/test/1.0.0"s;

  MockFunction<void(ListenerManager::StreamResult)> cb;

  EXPECT_CALL(cb, Call(Eq(outcome::success(stream)))).Times(1);

  EXPECT_CALL(*router, setProtocolHandler(Eq(p), _, _))
      .WillOnce(Arg1CallbackWithArg(stream));

  listener->setProtocolHandler(p, [&](auto &&s) { cb.Call(s); },
                               [p](auto &&proto) { return proto == p; });
}
