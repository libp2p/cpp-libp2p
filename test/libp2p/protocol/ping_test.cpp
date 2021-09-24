/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/ping.hpp"

#include <gtest/gtest.h>
#include <boost/asio/io_service.hpp>
#include <boost/optional.hpp>
#include <libp2p/common/literals.hpp>
#include "libp2p/event/bus.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/protocol/ping/common.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/crypto/random_generator_mock.hpp"
#include "mock/libp2p/host/host_mock.hpp"
#include "mock/libp2p/peer/peer_repository_mock.hpp"

using namespace libp2p;
using namespace protocol;
using namespace crypto::random;
using namespace connection;
using namespace protocol::detail;
using namespace peer;
using namespace common;

using testing::_;
using testing::InvokeArgument;
using testing::Return;
using testing::ReturnRef;

using std::chrono_literals::operator""ms;

class PingTest : public testing::Test {
 public:
  static constexpr uint32_t kPingMsgSize = 32;

  boost::asio::io_context io_context_;
  libp2p::event::Bus bus_;

  HostMock host_;
  std::shared_ptr<RandomGeneratorMock> rand_gen_ =
      std::make_shared<RandomGeneratorMock>();

  std::shared_ptr<Ping> ping_ = std::make_shared<Ping>(
      host_, bus_, io_context_, rand_gen_, PingConfig{1, kPingMsgSize});

  std::shared_ptr<CapableConnectionMock> conn_ =
      std::make_shared<CapableConnectionMock>();
  std::shared_ptr<StreamMock> stream_ = std::make_shared<StreamMock>();

  PeerId peer_id_ = "xxxMyPeerxxx"_peerid;
  PeerInfo peer_info_{peer_id_, {}};
  PeerRepositoryMock peer_repo_;

  std::vector<uint8_t> buffer_ = std::vector<uint8_t>(kPingMsgSize, 0xE3);
};

ACTION_P(ReadPut, buf) {
  std::copy(buf.begin(), buf.end(), arg0.begin());
  arg2(arg0.size());
}

/**
 * @given Ping protocol handler
 * @when a stream over the Ping protocol arrives
 * @then a new session is created @and reads a Ping message from the stream @and
 * writes it back
 */
TEST_F(PingTest, PingServer) {
  EXPECT_CALL(*stream_, read(_, kPingMsgSize, _))
      .WillOnce(ReadPut(buffer_))
      .WillOnce(  // no second read
          InvokeArgument<2>(outcome::failure(boost::system::error_code{})));
  EXPECT_CALL(*stream_,
              write(gsl::span<const uint8_t>(buffer_), kPingMsgSize, _))
      .WillOnce(InvokeArgument<2>(buffer_.size()));

  EXPECT_CALL(*stream_, isClosedForWrite()).WillOnce(Return(false));
  EXPECT_CALL(*stream_, isClosedForRead())
      .Times(2)
      .WillRepeatedly(Return(false));

  ping_->handle(stream_);
}

/**
 * @given Ping protocol handler
 * @when a stream over the Ping protocol is initiated from our side
 * @then a Ping message is sent over that stream @and we expect to get it back
 */
TEST_F(PingTest, PingClient) {
  EXPECT_CALL(*conn_, remotePeer()).WillOnce(Return(peer_id_));
  EXPECT_CALL(host_, getPeerRepository()).WillOnce(ReturnRef(peer_repo_));
  EXPECT_CALL(peer_repo_, getPeerInfo(peer_id_)).WillOnce(Return(peer_info_));
  EXPECT_CALL(host_, newStream(peer_info_, kPingProto, _, _))
      .WillOnce(InvokeArgument<2>(stream_));

  EXPECT_CALL(*rand_gen_, randomBytes(kPingMsgSize))
      .Times(2)
      .WillRepeatedly(Return(buffer_));
  EXPECT_CALL(*stream_,
              write(gsl::span<const uint8_t>(buffer_), kPingMsgSize, _))
      .WillOnce(InvokeArgument<2>(buffer_.size()))
      .WillOnce(  // no second write
          InvokeArgument<2>(outcome::failure(boost::system::error_code{
              boost::asio::error::invalid_argument})));
  EXPECT_CALL(*stream_, read(_, kPingMsgSize, _)).WillOnce(ReadPut(buffer_));

  EXPECT_CALL(*stream_, isClosedForWrite())
      .Times(2)
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*stream_, isClosedForRead()).WillOnce(Return(false));

  EXPECT_CALL(*stream_, remotePeerId()).WillOnce(Return(peer_id_));

  ping_->startPinging(conn_,
                      [](auto &&session_res) { ASSERT_TRUE(session_res); });

  io_context_.run_for(100ms);
}

/**
 * @given Ping protocol handler
 * @when a stream over the Ping protocol is initiated from our side @and the
 * other side does not respond in timeout time
 * @then PingIsDead event is emitted over the bus
 */
TEST_F(PingTest, PingClientTimeoutExpired) {
  EXPECT_CALL(*conn_, remotePeer()).WillOnce(Return(peer_id_));
  EXPECT_CALL(host_, getPeerRepository()).WillOnce(ReturnRef(peer_repo_));
  EXPECT_CALL(peer_repo_, getPeerInfo(peer_id_)).WillOnce(Return(peer_info_));
  EXPECT_CALL(host_, newStream(peer_info_, kPingProto, _, _))
      .WillOnce(InvokeArgument<2>(stream_));

  EXPECT_CALL(*rand_gen_, randomBytes(kPingMsgSize)).WillOnce(Return(buffer_));
  EXPECT_CALL(*stream_,
              write(gsl::span<const uint8_t>(buffer_), kPingMsgSize, _));

  EXPECT_CALL(*stream_, isClosedForWrite()).WillOnce(Return(false));

  EXPECT_CALL(*stream_, remotePeerId()).WillOnce(Return(peer_id_));

  boost::optional<peer::PeerId> dead_peer_id;
  auto h = bus_.getChannel<event::protocol::PeerIsDeadChannel>().subscribe(
      [&dead_peer_id](auto &&peer_id) mutable { dead_peer_id = peer_id; });

  ping_->startPinging(conn_,
                      [](auto &&session_res) { ASSERT_TRUE(session_res); });

  io_context_.run_for(100ms);

  ASSERT_TRUE(dead_peer_id);
  ASSERT_EQ(*dead_peer_id, peer_id_);
}
