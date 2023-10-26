/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/upgrader_impl.hpp"

#include <unordered_map>

#include <gtest/gtest.h>
#include <testutil/gmock_actions.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/multi/multiaddress_protocol_list.hpp"
#include "libp2p/multi/multihash.hpp"
#include "libp2p/transport/tcp/tcp_util.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/layer_connection_mock.hpp"
#include "mock/libp2p/connection/raw_connection_mock.hpp"
#include "mock/libp2p/connection/secure_connection_mock.hpp"
#include "mock/libp2p/layer/layer_adaptor_mock.hpp"
#include "mock/libp2p/muxer/muxer_adaptor_mock.hpp"
#include "mock/libp2p/protocol_muxer/protocol_muxer_mock.hpp"
#include "mock/libp2p/security/security_adaptor_mock.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::transport;
using namespace libp2p::muxer;
using namespace libp2p::layer;
using namespace libp2p::security;
using namespace libp2p::peer;
using namespace libp2p::connection;
using namespace libp2p::protocol_muxer;
using namespace libp2p::basic;
using namespace libp2p::multi;

using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::Truly;

using libp2p::outcome::failure;
using libp2p::outcome::success;

class UpgraderTest : public testing::Test {
 protected:
  void SetUp() override {
    for (size_t i = 0; i < layer_adaptors_.size(); ++i) {
      ON_CALL(*std::static_pointer_cast<LayerAdaptorMock>(layer_adaptors_[i]),
              getProtocol())
          .WillByDefault(Return(layer_protos_[i]));
    }
    for (size_t i = 0; i < security_protos_.size(); ++i) {
      ON_CALL(
          *std::static_pointer_cast<SecurityAdaptorMock>(security_adaptors_[i]),
          getProtocolId())
          .WillByDefault(Return(security_protos_[i]));
    }
    for (size_t i = 0; i < muxer_protos_.size(); ++i) {
      ON_CALL(*std::static_pointer_cast<MuxerAdaptorMock>(muxer_adaptors_[i]),
              getProtocolId())
          .WillByDefault(Return(muxer_protos_[i]));
    }

    upgrader_ = std::make_shared<UpgraderImpl>(
        muxer_, layer_adaptors_, security_adaptors_, muxer_adaptors_);
  }

  PeerId peer_id_ = testutil::randomPeerId();

  std::shared_ptr<ProtocolMuxerMock> muxer_ =
      std::make_shared<ProtocolMuxerMock>();

  std::vector<std::shared_ptr<LayerAdaptor>> layer_adaptors_{
      std::make_shared<NiceMock<LayerAdaptorMock>>(),
      std::make_shared<NiceMock<LayerAdaptorMock>>()};

  std::vector<libp2p::multi::Protocol::Code> layer_protos_{
      libp2p::multi::Protocol::Code::_DUMMY_PROTO_1,
      libp2p::multi::Protocol::Code::_DUMMY_PROTO_2,
  };
  std::vector<ProtocolName> security_protos_{"security_proto1",
                                             "security_proto2"};
  std::vector<std::shared_ptr<SecurityAdaptor>> security_adaptors_{
      std::make_shared<NiceMock<SecurityAdaptorMock>>(),
      std::make_shared<NiceMock<SecurityAdaptorMock>>()};

  std::vector<ProtocolName> muxer_protos_{"muxer_proto1", "muxer_proto2"};
  std::vector<std::shared_ptr<MuxerAdaptor>> muxer_adaptors_{
      std::make_shared<NiceMock<MuxerAdaptorMock>>(),
      std::make_shared<NiceMock<MuxerAdaptorMock>>()};

  std::shared_ptr<Upgrader> upgrader_;

  std::shared_ptr<RawConnectionMock> raw_conn_ =
      std::make_shared<NiceMock<RawConnectionMock>>();
  std::shared_ptr<LayerConnectionMock> layer1_conn_ =
      std::make_shared<NiceMock<LayerConnectionMock>>();
  std::shared_ptr<LayerConnectionMock> layer2_conn_ =
      std::make_shared<NiceMock<LayerConnectionMock>>();
  std::shared_ptr<SecureConnectionMock> sec_conn_ =
      std::make_shared<NiceMock<SecureConnectionMock>>();
  std::shared_ptr<CapableConnectionMock> muxed_conn_ =
      std::make_shared<NiceMock<CapableConnectionMock>>();

  void setAllInbound() {
    EXPECT_CALL(*raw_conn_, isInitiator_hack()).WillRepeatedly(Return(false));
    EXPECT_CALL(*layer1_conn_, isInitiator_hack())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*layer2_conn_, isInitiator_hack())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*sec_conn_, isInitiator_hack()).WillRepeatedly(Return(false));
    EXPECT_CALL(*muxed_conn_, isInitiator_hack()).WillRepeatedly(Return(false));
  }

  void setAllOutbound() {
    EXPECT_CALL(*raw_conn_, isInitiator_hack()).WillRepeatedly(Return(true));
    EXPECT_CALL(*layer1_conn_, isInitiator_hack()).WillRepeatedly(Return(true));
    EXPECT_CALL(*layer2_conn_, isInitiator_hack()).WillRepeatedly(Return(true));
    EXPECT_CALL(*sec_conn_, isInitiator_hack()).WillRepeatedly(Return(true));
    EXPECT_CALL(*muxed_conn_, isInitiator_hack()).WillRepeatedly(Return(true));
  }
};

TEST_F(UpgraderTest, UpgradeLayersInitiator) {
  setAllOutbound();

  ASSERT_OUTCOME_SUCCESS(
      address,
      libp2p::multi::Multiaddress::create(
          "/ip4/127.0.0.1/tcp/1234/_dummy_proto_1/_dummy_proto_2"
          "/p2p/12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV"));
  auto layers = detail::getLayers(address);

  EXPECT_CALL(*std::static_pointer_cast<LayerAdaptorMock>(layer_adaptors_[0]),
              upgradeOutbound(
                  _, std::static_pointer_cast<LayerConnection>(raw_conn_), _))
      .WillOnce(Arg2CallbackWithArg(layer1_conn_));

  EXPECT_CALL(
      *std::static_pointer_cast<LayerAdaptorMock>(layer_adaptors_[1]),
      upgradeOutbound(
          _, std::static_pointer_cast<LayerConnection>(layer1_conn_), _))
      .WillOnce(Arg2CallbackWithArg(layer2_conn_));

  upgrader_->upgradeLayersOutbound(
      address, raw_conn_, layers, [this](auto &&upgraded_conn_res) {
        ASSERT_OUTCOME_SUCCESS(upgraded_conn, upgraded_conn_res);
        ASSERT_EQ(upgraded_conn, layer2_conn_);
      });
}

TEST_F(UpgraderTest, UpgradeLayersNotInitiator) {
  setAllInbound();

  ASSERT_OUTCOME_SUCCESS(
      address,
      libp2p::multi::Multiaddress::create(
          "/ip4/127.0.0.1/tcp/1234/_dummy_proto_1/_dummy_proto_2"
          "/p2p/12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV"));
  auto layers = detail::getLayers(address);

  EXPECT_CALL(
      *std::static_pointer_cast<LayerAdaptorMock>(layer_adaptors_[0]),
      upgradeInbound(std::static_pointer_cast<LayerConnection>(raw_conn_), _))
      .WillOnce(Arg1CallbackWithArg(layer1_conn_));

  EXPECT_CALL(*std::static_pointer_cast<LayerAdaptorMock>(layer_adaptors_[1]),
              upgradeInbound(
                  std::static_pointer_cast<LayerConnection>(layer1_conn_), _))
      .WillOnce(Arg1CallbackWithArg(layer2_conn_));

  upgrader_->upgradeLayersInbound(
      raw_conn_, layers, [this](auto &&upgraded_conn_res) {
        ASSERT_TRUE(upgraded_conn_res);
        ASSERT_EQ(upgraded_conn_res.value(), layer2_conn_);
      });
}

TEST_F(UpgraderTest, UpgradeSecureInitiator) {
  setAllOutbound();

  auto if_sec_proto = [&](std::span<const ProtocolName> actual) {
    auto expected = std::span<const ProtocolName>(security_protos_);
    return std::equal(
        actual.begin(), actual.end(), expected.begin(), expected.end());
  };

  EXPECT_CALL(*muxer_,
              selectOneOf(Truly(if_sec_proto),
                          std::static_pointer_cast<ReadWriter>(layer2_conn_),
                          true,
                          true,
                          _))
      .WillOnce(Arg4CallbackWithArg(security_protos_[0]));
  EXPECT_CALL(
      *std::static_pointer_cast<SecurityAdaptorMock>(security_adaptors_[0]),
      secureOutbound(
          std::static_pointer_cast<LayerConnection>(layer2_conn_), peer_id_, _))
      .WillOnce(Arg2CallbackWithArg(sec_conn_));

  upgrader_->upgradeToSecureOutbound(
      layer2_conn_, peer_id_, [this](auto &&upgraded_conn_res) {
        ASSERT_TRUE(upgraded_conn_res);
        ASSERT_EQ(upgraded_conn_res.value(), sec_conn_);
      });
}

TEST_F(UpgraderTest, UpgradeSecureNotInitiator) {
  setAllInbound();

  auto if_sec_proto = [&](std::span<const ProtocolName> actual) {
    auto expected = std::span<const ProtocolName>(security_protos_);
    return std::equal(
        actual.begin(), actual.end(), expected.begin(), expected.end());
  };

  EXPECT_CALL(*muxer_,
              selectOneOf(Truly(if_sec_proto),
                          std::static_pointer_cast<ReadWriter>(layer2_conn_),
                          false,
                          true,
                          _))
      .WillOnce(Arg4CallbackWithArg(success(security_protos_[1])));
  EXPECT_CALL(
      *std::static_pointer_cast<SecurityAdaptorMock>(security_adaptors_[1]),
      secureInbound(std::static_pointer_cast<LayerConnection>(layer2_conn_), _))
      .WillOnce(Arg1CallbackWithArg(success(sec_conn_)));

  upgrader_->upgradeToSecureInbound(
      layer2_conn_, [this](auto &&upgraded_conn_res) {
        ASSERT_TRUE(upgraded_conn_res);
        ASSERT_EQ(upgraded_conn_res.value(), sec_conn_);
      });
}

TEST_F(UpgraderTest, UpgradeSecureFail) {
  setAllInbound();

  auto if_sec_proto = [&](std::span<const ProtocolName> actual) {
    auto expected = std::span<const ProtocolName>(security_protos_);
    return std::equal(
        actual.begin(), actual.end(), expected.begin(), expected.end());
  };

  EXPECT_CALL(*muxer_,
              selectOneOf(Truly(if_sec_proto),
                          std::static_pointer_cast<ReadWriter>(layer2_conn_),
                          false,
                          true,
                          _))
      .WillOnce(Arg4CallbackWithArg(failure(std::error_code())));

  upgrader_->upgradeToSecureInbound(layer2_conn_, [](auto &&upgraded_conn_res) {
    ASSERT_FALSE(upgraded_conn_res);
  });
}

TEST_F(UpgraderTest, UpgradeMux) {
  setAllOutbound();

  auto if_muxer_proto = [&](std::span<const ProtocolName> actual) {
    auto expected = std::span<const ProtocolName>(muxer_protos_);
    return std::equal(
        actual.begin(), actual.end(), expected.begin(), expected.end());
  };

  EXPECT_CALL(*muxer_,
              selectOneOf(Truly(if_muxer_proto),
                          std::static_pointer_cast<ReadWriter>(sec_conn_),
                          true,
                          true,
                          _))
      .WillOnce(Arg4CallbackWithArg(success(muxer_protos_[0])));
  EXPECT_CALL(
      *std::static_pointer_cast<MuxerAdaptorMock>(muxer_adaptors_[0]),
      muxConnection(std::static_pointer_cast<SecureConnection>(sec_conn_), _))
      .WillOnce(Arg1CallbackWithArg(muxed_conn_));

  upgrader_->upgradeToMuxed(sec_conn_, [this](auto &&upgraded_conn_res) {
    ASSERT_TRUE(upgraded_conn_res);
    ASSERT_EQ(upgraded_conn_res.value(), muxed_conn_);
  });
}

TEST_F(UpgraderTest, UpgradeMuxFail) {
  setAllOutbound();

  auto if_muxer_proto = [&](std::span<const ProtocolName> actual) {
    auto expected = std::span<const ProtocolName>(muxer_protos_);
    return std::equal(
        actual.begin(), actual.end(), expected.begin(), expected.end());
  };

  EXPECT_CALL(*muxer_,
              selectOneOf(Truly(if_muxer_proto),
                          std::static_pointer_cast<ReadWriter>(sec_conn_),
                          true,
                          true,
                          _))
      .WillOnce(Arg4CallbackWithArg(failure(std::error_code())));

  upgrader_->upgradeToMuxed(sec_conn_, [](auto &&upgraded_conn_res) {
    ASSERT_FALSE(upgraded_conn_res);
  });
}
