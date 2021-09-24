/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/identify.hpp>

#include <vector>

#include <generated/protocol/identify/protobuf/identify.pb.h>
#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/network/connection_manager.hpp>
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "mock/libp2p/host/host_mock.hpp"
#include "mock/libp2p/network/connection_manager_mock.hpp"
#include "mock/libp2p/network/listener_mock.hpp"
#include "mock/libp2p/network/network_mock.hpp"
#include "mock/libp2p/network/router_mock.hpp"
#include "mock/libp2p/peer/address_repository_mock.hpp"
#include "mock/libp2p/peer/identity_manager_mock.hpp"
#include "mock/libp2p/peer/key_repository_mock.hpp"
#include "mock/libp2p/peer/peer_repository_mock.hpp"
#include "mock/libp2p/peer/protocol_repository_mock.hpp"
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
using testing::NiceMock;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;

class IdentifyTest : public testing::Test {
 public:
  void SetUp() override {
    testutil::prepareLoggers();

    // create a Protobuf message, which is to be "read" or written
    for (const auto &proto : protocols_) {
      identify_pb_msg_.add_protocols(proto);
    }
    identify_pb_msg_.set_observedaddr(
        std::string(remote_multiaddr_.getBytesAddress().begin(),
                    remote_multiaddr_.getBytesAddress().end()));
    for (const auto &addr : listen_addresses_) {
      identify_pb_msg_.add_listenaddrs(std::string(
          addr.getBytesAddress().begin(), addr.getBytesAddress().end()));
    }
    identify_pb_msg_.set_publickey(marshalled_pubkey_.data(),
                                   marshalled_pubkey_.size());
    identify_pb_msg_.set_protocolversion(kLibp2pVersion);
    identify_pb_msg_.set_agentversion(kClientVersion);

    pb_msg_len_varint_ =
        std::make_shared<UVarint>(identify_pb_msg_.ByteSizeLong());

    identify_pb_msg_bytes_.insert(
        identify_pb_msg_bytes_.end(),
        std::make_move_iterator(pb_msg_len_varint_->toVector().begin()),
        std::make_move_iterator(pb_msg_len_varint_->toVector().end()));
    identify_pb_msg_bytes_.insert(identify_pb_msg_bytes_.end(),
                                  identify_pb_msg_.ByteSizeLong(), 0);
    identify_pb_msg_.SerializeToArray(
        identify_pb_msg_bytes_.data() + pb_msg_len_varint_->size(),
        identify_pb_msg_.ByteSizeLong());

    id_msg_processor_ = std::make_shared<IdentifyMessageProcessor>(
        host_, conn_manager_, id_manager_, key_marshaller_);
    identify_ = std::make_shared<Identify>(host_, id_msg_processor_, bus_);
  }

  HostMock host_;
  libp2p::event::Bus bus_;
  IdentityManagerMock id_manager_;
  std::shared_ptr<marshaller::KeyMarshaller> key_marshaller_ =
      std::make_shared<marshaller::KeyMarshallerMock>();

  std::shared_ptr<IdentifyMessageProcessor> id_msg_processor_;
  std::shared_ptr<Identify> identify_;

  std::shared_ptr<CapableConnectionMock> connection_ =
      std::make_shared<CapableConnectionMock>();
  std::shared_ptr<NiceMock<StreamMock>> stream_ =
      std::make_shared<NiceMock<StreamMock>>();

  // mocked host's components
  RouterMock router_;

  // Identify Protobuf message and its components
  identify::pb::Identify identify_pb_msg_;
  std::vector<uint8_t> identify_pb_msg_bytes_;
  std::shared_ptr<UVarint> pb_msg_len_varint_;

  std::vector<peer::Protocol> protocols_{"/http/5.0.1", "/dogeproto/2.2.8"};

  std::vector<multi::Multiaddress> listen_addresses_{
      "/ip4/1.1.1.1/tcp/1001"_multiaddr, "/ip4/1.1.1.1/tcp/1002"_multiaddr};

  multi::Multiaddress observer_address_ = "/ip4/1.1.1.1/tcp/1234"_multiaddr;

  std::vector<uint8_t> marshalled_pubkey_{0x11, 0x22, 0x33, 0x44};
  std::vector<uint8_t> pubkey_data_{0x55, 0x66, 0x77, 0x88};
  PublicKey pubkey_{{Key::Type::RSA, pubkey_data_}};
  KeyPair key_pair_{pubkey_, PrivateKey{}};

  const peer::PeerId kRemotePeerId =
      PeerId::fromPublicKey(ProtobufKey{marshalled_pubkey_}).value();
  multi::Multiaddress remote_multiaddr_ = "/ip4/2.2.2.2/tcp/1234"_multiaddr;
  const peer::PeerInfo kRemotePeerInfo{
      kRemotePeerId, std::vector<multi::Multiaddress>{remote_multiaddr_}};

  const peer::PeerId kOwnPeerId =
      PeerId::fromPublicKey(ProtobufKey{marshalled_pubkey_}).value();
  const peer::PeerInfo kOwnPeerInfo{kOwnPeerId, listen_addresses_};

  const std::string kLibp2pVersion = "ipfs/0.1.0";
  const std::string kClientVersion = "cpp-libp2p/0.1.0";

  PeerRepositoryMock peer_repo_;
  ProtocolRepositoryMock proto_repo_;
  KeyRepositoryMock key_repo_;
  AddressRepositoryMock addr_repo_;

  NetworkMock network_;
  ListenerMock listener_;
  ConnectionManagerMock conn_manager_;

  const std::string kIdentifyProto = "/ipfs/id/1.0.0";
};

ACTION_P2(Success, buf, res) {
  // better compare here, as this will show diff
  ASSERT_EQ(arg0, buf);
  ASSERT_EQ(arg1, buf.size());
  arg2(std::move(res));
}

ACTION_P(Close, res) {
  arg0(std::move(res));
}

/**
 * @given Identify object
 * @when a stream over Identify protocol is opened from another side
 * @then well-formed Identify message is sent by our peer
 */
TEST_F(IdentifyTest, Send) {
  // setup components, so that when Identify asks them, they give expected
  // parameters to be put into the Protobuf message
  EXPECT_CALL(host_, getRouter()).WillOnce(ReturnRef(router_));
  EXPECT_CALL(router_, getSupportedProtocols()).WillOnce(Return(protocols_));

  EXPECT_CALL(*stream_, remotePeerId()).WillRepeatedly(Return(kRemotePeerId));

  EXPECT_CALL(*stream_, remoteMultiaddr())
      .WillRepeatedly(Return(outcome::success(remote_multiaddr_)));

  EXPECT_CALL(host_, getPeerInfo()).WillOnce(Return(kOwnPeerInfo));

  EXPECT_CALL(id_manager_, getKeyPair()).WillOnce(ReturnRef(Const(key_pair_)));
  EXPECT_CALL(
      *std::static_pointer_cast<marshaller::KeyMarshallerMock>(key_marshaller_),
      marshal(pubkey_))
      .WillOnce(Return(ProtobufKey{marshalled_pubkey_}));

  EXPECT_CALL(host_, getLibp2pVersion()).WillOnce(Return(kLibp2pVersion));
  EXPECT_CALL(host_, getLibp2pClientVersion()).WillOnce(Return(kClientVersion));

  // handle Identify request and check it
  EXPECT_CALL(*stream_, write(_, _, _))
      .WillOnce(Success(gsl::span<const uint8_t>(identify_pb_msg_bytes_.data(),
                                                 identify_pb_msg_bytes_.size()),
                        outcome::success(identify_pb_msg_bytes_.size())));

  identify_->handle(std::static_pointer_cast<Stream>(stream_));
}

ACTION_P(ReadPut, buf) {
  std::copy(buf.begin(), buf.end(), arg0.begin());
  arg2(buf.size());
}

ACTION_P(ReturnStreamRes, s) {
  arg2(outcome::success(std::move(s)));
}

/**
 * @given Identify object
 * @when a new connection event is triggered
 * @then Identify opens a new stream over that connection @and requests other
 * peer to be identified @and accepts the received message
 */
TEST_F(IdentifyTest, Receive) {
  EXPECT_CALL(host_, setProtocolHandler(kIdentifyProto, _)).WillOnce(Return());

  EXPECT_CALL(*connection_, remotePeer()).WillOnce(Return(kRemotePeerId));
  EXPECT_CALL(*connection_, remoteMultiaddr())
      .WillOnce(Return(remote_multiaddr_));

  EXPECT_CALL(host_, newStream(kRemotePeerInfo, kIdentifyProto, _, _))
      .WillOnce(ReturnStreamRes(std::static_pointer_cast<Stream>(stream_)));

  EXPECT_CALL(*stream_, read(_, 1, _))
      .WillOnce(ReadPut(gsl::make_span(identify_pb_msg_bytes_.data(), 1)));
  EXPECT_CALL(*stream_, read(_, pb_msg_len_varint_->toUInt64(), _))
      .WillOnce(ReadPut(gsl::make_span(
          identify_pb_msg_bytes_.data() + pb_msg_len_varint_->size(),
          identify_pb_msg_bytes_.size() - pb_msg_len_varint_->size())));

  EXPECT_CALL(*stream_, remotePeerId())
      .Times(2)
      .WillRepeatedly(Return(kRemotePeerId));
  EXPECT_CALL(*stream_, remoteMultiaddr())
      .Times(2)
      .WillRepeatedly(Return(outcome::success(remote_multiaddr_)));

  EXPECT_CALL(*stream_, close(_)).WillOnce(Close(outcome::success()));

  // consumePublicKey
  EXPECT_CALL(
      *std::static_pointer_cast<marshaller::KeyMarshallerMock>(key_marshaller_),
      unmarshalPublicKey(ProtobufKey{marshalled_pubkey_}))
      .WillOnce(Return(pubkey_));

  EXPECT_CALL(host_, getPeerRepository())
      .Times(3)
      .WillRepeatedly(ReturnRef(peer_repo_));
  EXPECT_CALL(peer_repo_, getKeyRepository()).WillOnce(ReturnRef(key_repo_));
  EXPECT_CALL(key_repo_, addPublicKey(kRemotePeerId, pubkey_))
      .WillOnce(Return(outcome::success()));

  EXPECT_CALL(peer_repo_, getProtocolRepository())
      .WillOnce(ReturnRef(proto_repo_));
  EXPECT_CALL(
      proto_repo_,
      addProtocols(kRemotePeerId, gsl::span<const peer::Protocol>(protocols_)))
      .WillOnce(Return(outcome::success()));

  // consumeObservedAddresses
  EXPECT_CALL(*stream_, localMultiaddr())
      .WillOnce(Return(listen_addresses_[0]));
  EXPECT_CALL(*stream_, isInitiator()).WillOnce(Return(true));

  EXPECT_CALL(host_, getNetwork()).Times(1).WillRepeatedly(ReturnRef(network_));
  EXPECT_CALL(network_, getListener())
      .Times(1)
      .WillRepeatedly(ReturnRef(listener_));

  EXPECT_CALL(listener_, getListenAddressesInterfaces())
      .WillOnce(Return(std::vector<Multiaddress>{}));
  EXPECT_CALL(listener_, getListenAddresses())
      .WillOnce(Return(listen_addresses_));

  EXPECT_CALL(host_, getAddresses()).WillOnce(Return(listen_addresses_));

  // consumeListenAddresses
  EXPECT_CALL(peer_repo_, getAddressRepository())
      .WillOnce(ReturnRef(addr_repo_));
  EXPECT_CALL(
      addr_repo_,
      updateAddresses(kRemotePeerId,
                      std::chrono::duration_cast<std::chrono::milliseconds>(
                          peer::ttl::kTransient)))
      .WillOnce(Return(outcome::success()));

  EXPECT_CALL(addr_repo_, getAddresses(kRemotePeerId))
      .WillOnce(Return(std::vector<multi::Multiaddress>{remote_multiaddr_}));

  peer::PeerInfo pinfo = {kRemotePeerId, {remote_multiaddr_}};

  EXPECT_CALL(conn_manager_, getBestConnectionForPeer(kRemotePeerId))
      .WillOnce(Return(connection_));
  EXPECT_CALL(
      addr_repo_,
      upsertAddresses(kRemotePeerId,
                      gsl::span<const multi::Multiaddress>(listen_addresses_),
                      peer::ttl::kPermanent))
      .WillOnce(Return(outcome::success()));

  // trigger the event, to which Identify object reacts
  identify_->start();
  bus_.getChannel<event::network::OnNewConnectionChannel>().publish(
      std::weak_ptr<CapableConnection>(connection_));
}
