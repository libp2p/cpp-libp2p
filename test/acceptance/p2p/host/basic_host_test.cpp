/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/host/basic_host/basic_host.hpp>

#include <gtest/gtest.h>

#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/network/dialer_mock.hpp"
#include "mock/libp2p/network/listener_mock.hpp"
#include "mock/libp2p/network/network_mock.hpp"
#include "mock/libp2p/network/transport_manager_mock.hpp"
#include "mock/libp2p/peer/address_repository_mock.hpp"
#include "mock/libp2p/peer/identity_manager_mock.hpp"
#include "mock/libp2p/peer/peer_repository_mock.hpp"

#include <libp2p/common/literals.hpp>
#include "testutil/gmock_actions.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;
using namespace common;

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

struct BasicHostTest : public ::testing::Test {
  std::shared_ptr<connection::StreamMock> stream =
      std::make_shared<connection::StreamMock>();

  std::shared_ptr<peer::IdentityManagerMock> idmgr =
      std::make_shared<peer::IdentityManagerMock>();

  std::shared_ptr<network::ListenerMock> listener =
      std::make_shared<network::ListenerMock>();

  std::shared_ptr<network::DialerMock> dialer =
      std::make_shared<network::DialerMock>();

  std::shared_ptr<peer::AddressRepositoryMock> addr_repo =
      std::make_shared<peer::AddressRepositoryMock>();

  std::unique_ptr<Host> host = std::make_unique<host::BasicHost>(
      idmgr, std::make_unique<network::NetworkMock>(),
      std::make_unique<peer::PeerRepositoryMock>(),
      std::make_shared<libp2p::event::Bus>(),
      std::make_shared<libp2p::network::TransportManagerMock>());

  peer::PeerRepositoryMock &repo =
      (peer::PeerRepositoryMock &)host->getPeerRepository();

  network::NetworkMock &network = (network::NetworkMock &)host->getNetwork();

  /// VARS
  peer::PeerId id = "1"_peerid;

  multi::Multiaddress ma1 = "/ip4/1.3.3.7/udp/1"_multiaddr;
  multi::Multiaddress ma2 = "/ip4/1.3.3.7/udp/2"_multiaddr;
  multi::Multiaddress ma3 = "/ip4/1.3.3.7/udp/3"_multiaddr;
  multi::Multiaddress ma4 = "/ip4/1.3.3.7/udp/4"_multiaddr;
  std::vector<multi::Multiaddress> mas{ma1, ma2, ma3, ma4};
};

/**
 * @given default host
 * @when getId is called
 * @then peer's id is returned
 */
TEST_F(BasicHostTest, GetId) {
  EXPECT_CALL(*idmgr, getId()).WillOnce(ReturnRef(id));

  auto actual = host->getId();
  ASSERT_EQ(id, actual);
}

/**
 * @given default host
 * @when getPeerInfo is called
 * @then peer's info is returned
 */
TEST_F(BasicHostTest, GetPeerInfo) {
  EXPECT_CALL(network, getListener())
      .Times(2)
      .WillRepeatedly(ReturnRef(*listener));
  EXPECT_CALL(*idmgr, getId()).Times(2).WillRepeatedly(ReturnRef(id));
  EXPECT_CALL(repo, getAddressRepository()).WillOnce(ReturnRef(*addr_repo));
  EXPECT_CALL(*addr_repo, getAddresses(id)).WillOnce(Return(mas));
  EXPECT_CALL(*listener, getListenAddresses())
      .Times(1)
      .WillRepeatedly(Return(mas));
  EXPECT_CALL(*listener, getListenAddressesInterfaces())
      .Times(1)
      .WillRepeatedly(Return(mas));

  auto pinfo = host->getPeerInfo();
  auto expected = peer::PeerInfo{id, mas};
  ASSERT_EQ(pinfo, expected);
}

/**
 * @given default host
 * @when getAddresses is called
 * @then listen addresses are returned
 */
TEST_F(BasicHostTest, GetAddresses) {
  EXPECT_CALL(network, getListener()).WillOnce(ReturnRef(*listener));
  EXPECT_CALL(*listener, getListenAddresses()).WillOnce(Return(mas));

  auto addrs = host->getAddresses();
  ASSERT_EQ(addrs, mas);
}

/**
 * @given default host
 * @when getAddressesInterfaces is called
 * @then Listener.getListenAddressesInterfaces is called
 */
TEST_F(BasicHostTest, GetAddressesInterfaces) {
  EXPECT_CALL(network, getListener()).WillOnce(ReturnRef(*listener));
  EXPECT_CALL(*listener, getListenAddressesInterfaces()).WillOnce(Return(mas));

  auto addrs = host->getAddressesInterfaces();
  ASSERT_EQ(addrs, mas);
}

/**
 * @given default host
 * @when host connect's to other peer
 * @then dial is called once
 */
TEST_F(BasicHostTest, Connect) {
  peer::PeerInfo pinfo{"2"_peerid, {ma1}};
  EXPECT_CALL(network, getDialer()).WillOnce(ReturnRef(*dialer));
  EXPECT_CALL(*dialer, dial(pinfo, _, _)).Times(1);

  host->connect(pinfo);
}

/**
 * @given default host
 * @when host opens new stream to a remote host
 * @then newStream is called once
 */
TEST_F(BasicHostTest, NewStream) {
  peer::PeerInfo pinfo{"2"_peerid, {ma1}};
  peer::Protocol protocol = "/proto/1.0.0";

  EXPECT_CALL(network, getDialer()).WillOnce(ReturnRef(*dialer));
  EXPECT_CALL(*dialer,
              newStream(pinfo, protocol, _, std::chrono::milliseconds::zero()))
      .WillOnce(Arg2CallbackWithArg(stream));

  bool executed = false;
  host->newStream(pinfo, protocol, [&](auto &&result) {
    EXPECT_OUTCOME_TRUE(stream, result);
    (void)stream;
    executed = true;
  });

  ASSERT_TRUE(executed);
}
