/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/upgrader_impl.hpp"

#include <numeric>
#include <unordered_map>

#include <gtest/gtest.h>
#include <testutil/gmock_actions.hpp>
#include <testutil/outcome.hpp>
#include "libp2p/multi/multihash.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/raw_connection_mock.hpp"
#include "mock/libp2p/connection/secure_connection_mock.hpp"
#include "mock/libp2p/muxer/muxer_adaptor_mock.hpp"
#include "mock/libp2p/protocol_muxer/protocol_muxer_mock.hpp"
#include "mock/libp2p/security/security_adaptor_mock.hpp"
#include "testutil/libp2p/peer.hpp"

using namespace libp2p::transport;
using namespace libp2p::muxer;
using namespace libp2p::security;
using namespace libp2p::peer;
using namespace libp2p::connection;
using namespace libp2p::protocol_muxer;
using namespace libp2p::basic;

using testing::_;
using testing::NiceMock;
using testing::Return;

using libp2p::outcome::failure;
using libp2p::outcome::success;

class UpgraderTest : public testing::Test {
 protected:
  void SetUp() override {
    for (size_t i = 0; i < security_protos_.size(); ++i) {
      ON_CALL(
          *std::static_pointer_cast<SecurityAdaptorMock>(security_mocks_[i]),
          getProtocolId())
          .WillByDefault(Return(security_protos_[i]));
    }
    for (size_t i = 0; i < muxer_protos_.size(); ++i) {
      ON_CALL(*std::static_pointer_cast<MuxerAdaptorMock>(muxer_mocks_[i]),
              getProtocolId())
          .WillByDefault(Return(muxer_protos_[i]));
    }

    upgrader_ = std::make_shared<UpgraderImpl>(multiselect_mock_,
                                               security_mocks_, muxer_mocks_);
  }

  PeerId peer_id_ = testutil::randomPeerId();

  std::shared_ptr<ProtocolMuxerMock> multiselect_mock_ =
      std::make_shared<ProtocolMuxerMock>();

  std::vector<Protocol> security_protos_{"security_proto1", "security_proto2"};
  std::vector<std::shared_ptr<SecurityAdaptor>> security_mocks_{
      std::make_shared<NiceMock<SecurityAdaptorMock>>(),
      std::make_shared<NiceMock<SecurityAdaptorMock>>()};

  std::vector<Protocol> muxer_protos_{"muxer_proto1", "muxer_proto2"};
  std::vector<std::shared_ptr<MuxerAdaptor>> muxer_mocks_{
      std::make_shared<NiceMock<MuxerAdaptorMock>>(),
      std::make_shared<NiceMock<MuxerAdaptorMock>>()};

  std::shared_ptr<Upgrader> upgrader_;

  std::shared_ptr<RawConnectionMock> raw_conn_ =
      std::make_shared<NiceMock<RawConnectionMock>>();
  std::shared_ptr<SecureConnectionMock> sec_conn_ =
      std::make_shared<NiceMock<SecureConnectionMock>>();
  std::shared_ptr<CapableConnectionMock> muxed_conn_ =
      std::make_shared<NiceMock<CapableConnectionMock>>();
};

TEST_F(UpgraderTest, DISABLED_UpgradeSecureInitiator) {
  EXPECT_CALL(*raw_conn_, isInitiator_hack()).WillRepeatedly(Return(true));

  EXPECT_CALL(*multiselect_mock_,
              selectOneOf(gsl::span<const Protocol>(security_protos_),
                          std::static_pointer_cast<ReadWriter>(raw_conn_), true,
                          false, _))
      .WillOnce(Arg4CallbackWithArg(security_protos_[0]));
  EXPECT_CALL(
      *std::static_pointer_cast<SecurityAdaptorMock>(security_mocks_[0]),
      secureOutbound(std::static_pointer_cast<RawConnection>(raw_conn_),
                     peer_id_, _))
      .WillOnce(Arg2CallbackWithArg(sec_conn_));

  upgrader_->upgradeToSecureOutbound(
      raw_conn_, peer_id_, [this](auto &&upgraded_conn_res) {
        ASSERT_TRUE(upgraded_conn_res);
        ASSERT_EQ(upgraded_conn_res.value(), sec_conn_);
      });
}

TEST_F(UpgraderTest, DISABLED_UpgradeSecureNotInitiator) {
  EXPECT_CALL(*raw_conn_, isInitiator_hack()).WillRepeatedly(Return(false));
  EXPECT_CALL(*multiselect_mock_,
              selectOneOf(gsl::span<const Protocol>(security_protos_),
                          std::static_pointer_cast<ReadWriter>(raw_conn_),
                          false, false, _))
      .WillOnce(Arg4CallbackWithArg(success(security_protos_[1])));
  EXPECT_CALL(
      *std::static_pointer_cast<SecurityAdaptorMock>(security_mocks_[1]),
      secureInbound(std::static_pointer_cast<RawConnection>(raw_conn_), _))
      .WillOnce(Arg1CallbackWithArg(success(sec_conn_)));

  upgrader_->upgradeToSecureInbound(
      raw_conn_, [this](auto &&upgraded_conn_res) {
        ASSERT_TRUE(upgraded_conn_res);
        ASSERT_EQ(upgraded_conn_res.value(), sec_conn_);
      });
}

TEST_F(UpgraderTest, DISABLED_UpgradeSecureFail) {
  EXPECT_CALL(*raw_conn_, isInitiator_hack()).WillOnce(Return(false));
  EXPECT_CALL(*multiselect_mock_,
              selectOneOf(gsl::span<const Protocol>(security_protos_),
                          std::static_pointer_cast<ReadWriter>(raw_conn_),
                          false, false, _))
      .WillOnce(Arg4CallbackWithArg(failure(std::error_code())));

  upgrader_->upgradeToSecureInbound(raw_conn_, [](auto &&upgraded_conn_res) {
    ASSERT_FALSE(upgraded_conn_res);
  });
}

TEST_F(UpgraderTest, DISABLED_UpgradeMux) {
  EXPECT_CALL(*sec_conn_, isInitiatorMock()).WillOnce(Return(true));
  EXPECT_CALL(*multiselect_mock_,
              selectOneOf(gsl::span<const Protocol>(muxer_protos_),
                          std::static_pointer_cast<ReadWriter>(sec_conn_), true,
                          false, _))
      .WillOnce(Arg4CallbackWithArg(success(muxer_protos_[0])));
  EXPECT_CALL(
      *std::static_pointer_cast<MuxerAdaptorMock>(muxer_mocks_[0]),
      muxConnection(std::static_pointer_cast<SecureConnection>(sec_conn_), _))
      .WillOnce(Arg1CallbackWithArg(muxed_conn_));

  upgrader_->upgradeToMuxed(sec_conn_, [this](auto &&upgraded_conn_res) {
    ASSERT_TRUE(upgraded_conn_res);
    ASSERT_EQ(upgraded_conn_res.value(), muxed_conn_);
  });
}

TEST_F(UpgraderTest, DISABLED_UpgradeMuxFail) {
  EXPECT_CALL(*sec_conn_, isInitiatorMock()).WillOnce(Return(true));
  EXPECT_CALL(*multiselect_mock_,
              selectOneOf(gsl::span<const Protocol>(muxer_protos_),
                          std::static_pointer_cast<ReadWriter>(sec_conn_), true,
                          false, _))
      .WillOnce(Arg4CallbackWithArg(failure(std::error_code())));

  upgrader_->upgradeToMuxed(sec_conn_, [](auto &&upgraded_conn_res) {
    ASSERT_FALSE(upgraded_conn_res);
  });
}
