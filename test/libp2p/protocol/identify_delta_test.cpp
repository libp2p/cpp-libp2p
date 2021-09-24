/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/identify/identify_delta.hpp>

#include <generated/protocol/identify/protobuf/identify.pb.h>
#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/multi/uvarint.hpp>
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/host/host_mock.hpp"
#include "mock/libp2p/network/connection_manager_mock.hpp"
#include "mock/libp2p/peer/peer_repository_mock.hpp"
#include "mock/libp2p/peer/protocol_repository_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace libp2p;
using namespace peer;
using namespace crypto;
using namespace protocol;
using namespace network;
using namespace connection;
using namespace common;
using namespace multi;

using testing::_;
using testing::Const;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;

class IdentifyDeltaTest : public testing::Test {
 public:
  void SetUp() override {
    testutil::prepareLoggers();

    id_delta_ = std::make_shared<IdentifyDelta>(host_, conn_manager_, bus_);

    for (const auto &proto : added_protos_) {
      msg_added_protos_.mutable_delta()->add_added_protocols(proto);
      msg_added_rm_protos_.mutable_delta()->add_added_protocols(proto);
    }
    for (const auto &proto : removed_protos_) {
      msg_added_rm_protos_.mutable_delta()->add_rm_protocols(proto);
    }

    added_proto_len_ = UVarint{msg_added_protos_.ByteSizeLong()};
    msg_added_protos_bytes_.insert(msg_added_protos_bytes_.end(),
                                   added_proto_len_.toVector().begin(),
                                   added_proto_len_.toVector().end());
    msg_added_protos_bytes_.insert(msg_added_protos_bytes_.end(),
                                   msg_added_protos_.ByteSizeLong(), 0);
    msg_added_protos_.SerializeToArray(
        msg_added_protos_bytes_.data() + added_proto_len_.size(),
        msg_added_protos_.ByteSizeLong());

    added_rm_proto_len_ = UVarint{msg_added_rm_protos_.ByteSizeLong()};
    msg_added_rm_protos_bytes_.insert(msg_added_rm_protos_bytes_.end(),
                                      added_rm_proto_len_.toVector().begin(),
                                      added_rm_proto_len_.toVector().end());
    msg_added_rm_protos_bytes_.insert(msg_added_rm_protos_bytes_.end(),
                                      msg_added_rm_protos_.ByteSizeLong(), 0);
    msg_added_rm_protos_.SerializeToArray(
        msg_added_rm_protos_bytes_.data() + added_rm_proto_len_.size(),
        msg_added_rm_protos_.ByteSizeLong());
  }

  HostMock host_;
  libp2p::event::Bus bus_;

  std::shared_ptr<IdentifyDelta> id_delta_;

  std::vector<peer::Protocol> added_protos_{"/ping/1.0.0", "/ping/1.5.0"};
  std::vector<peer::Protocol> removed_protos_{"/http/5.2.8"};

  identify::pb::Identify msg_added_protos_;
  std::vector<uint8_t> msg_added_protos_bytes_;
  UVarint added_proto_len_{0};

  identify::pb::Identify msg_added_rm_protos_;
  std::vector<uint8_t> msg_added_rm_protos_bytes_;
  UVarint added_rm_proto_len_{0};

  ConnectionManagerMock conn_manager_;
  PeerRepositoryMock peer_repo_;
  ProtocolRepositoryMock proto_repo_;
  std::shared_ptr<CapableConnectionMock> conn_ =
      std::make_shared<CapableConnectionMock>();
  std::shared_ptr<StreamMock> stream_ = std::make_shared<StreamMock>();

  const std::string kIdentifyDeltaProtocol = "/p2p/id/delta/1.0.0";
  const PeerId kRemotePeerId = "xxxMyPeerIdxxx"_peerid;
  const PeerInfo kPeerInfo{
      kRemotePeerId,
      std::vector<multi::Multiaddress>{"/ip4/12.34.56.78/tcp/123"_multiaddr,
                                       "/ip4/192.168.0.1"_multiaddr}};
};

ACTION_P(InvokeLambda, s) {
  arg2(std::move(s));
}

/**
 * @given Identify-Delta
 * @when new protocols event is arrived
 * @then an Identify-Delta message with those protocols is sent over the network
 */
TEST_F(IdentifyDeltaTest, Send) {
  // getActivePeers
  EXPECT_CALL(conn_manager_, getConnections())
      .WillOnce(Return(std::vector<std::shared_ptr<CapableConnection>>{conn_}));
  EXPECT_CALL(*conn_, remotePeer()).WillOnce(Return(kRemotePeerId));
  EXPECT_CALL(host_, getPeerRepository()).WillOnce(ReturnRef(peer_repo_));
  EXPECT_CALL(peer_repo_, getPeerInfo(kRemotePeerId))
      .WillOnce(Return(kPeerInfo));

  // stream handling and message sending
  EXPECT_CALL(host_, newStream(kPeerInfo, kIdentifyDeltaProtocol, _, _))
      .WillOnce(InvokeLambda(stream_));
  EXPECT_CALL(*stream_,
              write(gsl::span<const uint8_t>(msg_added_protos_bytes_),
                    msg_added_protos_bytes_.size(), _))
      .WillOnce(InvokeLambda(outcome::success()));

  id_delta_->start();
  bus_.getChannel<event::network::ProtocolsAddedChannel>().publish(
      added_protos_);
}

ACTION_P(ReadPut, buf) {
  std::copy(buf.begin(), buf.end(), arg0.begin());
  arg2(buf.size());
}

/**
 * @given Identify-Delta
 * @when stream with Identify-Delta protocol was opened from the other side
 * @then receive and process Identify-Delta message
 */
TEST_F(IdentifyDeltaTest, Receive) {
  // handle
  EXPECT_CALL(*stream_, read(_, 1, _))
      .WillOnce(ReadPut(gsl::make_span(msg_added_rm_protos_bytes_.data(), 1)));
  EXPECT_CALL(*stream_, read(_, added_rm_proto_len_.toUInt64(), _))
      .WillOnce(ReadPut(gsl::make_span(
          msg_added_rm_protos_bytes_.data() + added_proto_len_.size(),
          msg_added_rm_protos_bytes_.size() - added_proto_len_.size())));

  // deltaReceived
  EXPECT_CALL(*stream_, remotePeerId())
      .Times(2)
      .WillRepeatedly(Return(kRemotePeerId));
  EXPECT_CALL(*stream_, remoteMultiaddr())
      .Times(1)
      .WillRepeatedly(Return(outcome::success(kPeerInfo.addresses[0])));

  EXPECT_CALL(host_, getPeerRepository()).WillOnce(ReturnRef(peer_repo_));
  EXPECT_CALL(peer_repo_, getProtocolRepository())
      .WillOnce(ReturnRef(proto_repo_));
  EXPECT_CALL(proto_repo_,
              addProtocols(kRemotePeerId,
                           gsl::span<const peer::Protocol>(added_protos_)))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(proto_repo_,
              removeProtocols(kRemotePeerId,
                              gsl::span<const peer::Protocol>(removed_protos_)))
      .WillOnce(Return(outcome::success()));

  id_delta_->handle(stream_);
}
