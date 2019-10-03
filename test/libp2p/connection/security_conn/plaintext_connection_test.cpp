/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/plaintext/plaintext_connection.hpp>

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <testutil/outcome.hpp>
#include "mock/libp2p/connection/raw_connection_mock.hpp"
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "testutil/gmock_actions.hpp"

using namespace libp2p::connection;
using namespace libp2p::basic;
using namespace libp2p::crypto;
using namespace libp2p::peer;
using namespace libp2p::common;

using testing::_;
using testing::ByMove;
using testing::IgnoreResult;
using testing::Return;

class PlaintextConnectionTest : public testing::Test {
 public:
  PublicKey local{{Key::Type::Secp256k1, {1}}};
  PublicKey remote{{Key::Type::Ed25519, {2}}};

  std::shared_ptr<RawConnectionMock> connection_ =
      std::make_shared<RawConnectionMock>();

  std::shared_ptr<marshaller::KeyMarshallerMock> key_marshaller_ =
      std::make_shared<marshaller::KeyMarshallerMock>();

  std::shared_ptr<SecureConnection> secure_connection_ =
      std::make_shared<PlaintextConnection>(connection_, local, remote,
                                            key_marshaller_);

  std::vector<uint8_t> bytes_{0x11, 0x22};
};

/**
 * @given plaintext secure connection
 * @when invoking localPeer method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, LocalPeer) {
  EXPECT_CALL(*key_marshaller_, marshal(local))
      .WillOnce(Return(ProtobufKey{local.data}));
  ASSERT_EQ(secure_connection_->localPeer().value(),
            PeerId::fromPublicKey(ProtobufKey{local.data}).value());
}

/**
 * @given plaintext secure connection
 * @when invoking remotePeer method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, RemotePeer) {
  EXPECT_CALL(*key_marshaller_, marshal(remote))
      .WillOnce(Return(ProtobufKey{remote.data}));
  ASSERT_EQ(secure_connection_->remotePeer().value(),
            PeerId::fromPublicKey(ProtobufKey{remote.data}).value());
}

/**
 * @given plaintext secure connection
 * @when invoking remotePublicKey method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, RemotePublicKey) {
  ASSERT_EQ(secure_connection_->remotePublicKey().value(), remote);
}

/**
 * @given plaintext secure connection
 * @when invoking isInitiator method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, IsInitiator) {
  EXPECT_CALL(*connection_, isInitiator_hack()).WillOnce(Return(true));
  ASSERT_TRUE(secure_connection_->isInitiator());
}

/**
 * @given plaintext secure connection
 * @when invoking localMultiaddr method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, LocalMultiaddr) {
  const auto kDefaultMultiaddr = "/ip4/192.168.0.1/tcp/226"_multiaddr;

  EXPECT_CALL(*connection_, localMultiaddr())
      .WillOnce(Return(kDefaultMultiaddr));
  EXPECT_OUTCOME_TRUE(ma, secure_connection_->localMultiaddr())
  ASSERT_EQ(ma.getStringAddress(), kDefaultMultiaddr.getStringAddress());
}

/**
 * @given plaintext secure connection
 * @when invoking remoteMultiaddr method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, RemoteMultiaddr) {
  const auto kDefaultMultiaddr = "/ip4/192.168.0.1/tcp/226"_multiaddr;

  EXPECT_CALL(*connection_, remoteMultiaddr())
      .WillOnce(Return(kDefaultMultiaddr));
  EXPECT_OUTCOME_TRUE(ma, secure_connection_->remoteMultiaddr())
  ASSERT_EQ(ma.getStringAddress(), kDefaultMultiaddr.getStringAddress());
}

/**
 * @given plaintext secure connection
 * @when invoking read method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, Read) {
  const int size = 100;
  EXPECT_CALL(*connection_, read(_, _, _)).WillOnce(AsioSuccess(size));
  auto buf = std::make_shared<std::vector<uint8_t>>(size, 0);
  secure_connection_->read(*buf, size, [size, buf](auto &&res) {
    ASSERT_TRUE(res) << res.error().message();
    ASSERT_EQ(res.value(), size);
  });
}

/**
 * @given plaintext secure connection
 * @when invoking readSome method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, ReadSome) {
  const int size = 100;
  const int smaller = 50;
  EXPECT_CALL(*connection_, readSome(_, _, _))
      .WillOnce(AsioSuccess(smaller /* less than 100 */));
  auto buf = std::make_shared<std::vector<uint8_t>>(size, 0);
  secure_connection_->readSome(*buf, smaller, [smaller, buf](auto &&res) {
    ASSERT_TRUE(res) << res.error().message();
    ASSERT_EQ(res.value(), smaller);
  });
}

/**
 * @given plaintext secure connection
 * @when invoking write method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, Write) {
  const int size = 100;
  EXPECT_CALL(*connection_, write(_, _, _)).WillOnce(AsioSuccess(size));
  auto buf = std::make_shared<std::vector<uint8_t>>(size, 0);
  secure_connection_->write(*buf, size, [size, buf](auto &&res) {
    ASSERT_TRUE(res) << res.error().message();
    ASSERT_EQ(res.value(), size);
  });
}

/**
 * @given plaintext secure connection
 * @when invoking writeSome method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, WriteSome) {
  const int size = 100;
  const int smaller = 50;
  EXPECT_CALL(*connection_, writeSome(_, _, _))
      .WillOnce(AsioSuccess(smaller /* less than 100 */));
  auto buf = std::make_shared<std::vector<uint8_t>>(size, 0);
  secure_connection_->writeSome(*buf, smaller, [smaller, buf](auto &&res) {
    ASSERT_TRUE(res) << res.error().message();
    ASSERT_EQ(res.value(), smaller);
  });
}

/**
 * @given plaintext secure connection
 * @when invoking isClosed method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, IsClosed) {
  EXPECT_CALL(*connection_, isClosed()).WillOnce(Return(false));
  ASSERT_FALSE(secure_connection_->isClosed());
}
