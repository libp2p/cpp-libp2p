/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/basic/read.hpp>
#include <libp2p/basic/write.hpp>
#include <libp2p/common/literals.hpp>
#include <libp2p/security/plaintext/plaintext_connection.hpp>
#include <qtils/test/outcome.hpp>
#include "mock/libp2p/connection/layer_connection_mock.hpp"
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "testutil/expect_read.hpp"
#include "testutil/expect_write.hpp"
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

  std::shared_ptr<LayerConnectionMock> connection_ =
      std::make_shared<LayerConnectionMock>();

  std::shared_ptr<marshaller::KeyMarshallerMock> key_marshaller_ =
      std::make_shared<marshaller::KeyMarshallerMock>();

  std::shared_ptr<SecureConnection> secure_connection_ =
      std::make_shared<PlaintextConnection>(
          connection_, local, remote, key_marshaller_);

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
  ASSERT_OUTCOME_SUCCESS(ma, secure_connection_->localMultiaddr());
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
  ASSERT_OUTCOME_SUCCESS(ma, secure_connection_->remoteMultiaddr());
  ASSERT_EQ(ma.getStringAddress(), kDefaultMultiaddr.getStringAddress());
}

/**
 * @given plaintext secure connection
 * @when invoking read method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, Read) {
  const int size = 100;
  EXPECT_CALL_READ(*connection_).WILL_READ_SIZE(size);
  auto buf = std::make_shared<std::vector<uint8_t>>(size, 0);
  libp2p::read(secure_connection_, *buf, [buf](outcome::result<void> res) {
    ASSERT_OUTCOME_SUCCESS(res);
  });
}

/**
 * @given plaintext secure connection
 * @when invoking write method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, Write) {
  const int size = 100;
  EXPECT_CALL_WRITE(*connection_).WILL_WRITE_SIZE(size);
  auto buf = std::make_shared<std::vector<uint8_t>>(size, 0);
  libp2p::write(secure_connection_, *buf, [buf](outcome::result<void> res) {
    ASSERT_OUTCOME_SUCCESS(res);
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
