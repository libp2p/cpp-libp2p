/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/security/plaintext/plaintext.hpp"

#include <gtest/gtest.h>
#include <boost/di.hpp>
#include <boost/di/extension/providers/mocks_provider.hpp>
#include <testutil/gmock_actions.hpp>
#include <testutil/outcome.hpp>
#include "common/hexutil.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "mock/libp2p/connection/raw_connection_mock.hpp"
#include "mock/libp2p/peer/identity_manager_mock.hpp"
#include "mock/libp2p/security/exchange_message_marshaller_mock.hpp"

using namespace libp2p;
using namespace connection;
using namespace security;
using namespace peer;
using namespace crypto;

using libp2p::peer::PeerId;
using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;

class PlaintextAdaptorTest : public testing::Test {
 public:
  std::shared_ptr<NiceMock<IdentityManagerMock>> idmgr =
      std::make_shared<NiceMock<IdentityManagerMock>>();

  std::shared_ptr<plaintext::ExchangeMessageMarshallerMock> marshaller =
      std::make_shared<plaintext::ExchangeMessageMarshallerMock>();

  std::shared_ptr<Plaintext> adaptor =
      std::make_shared<Plaintext>(marshaller, idmgr);

  void SetUp() override {
    ON_CALL(*conn, readSome(_, _, _)).WillByDefault(Arg2CallbackWithArg(5));
    ON_CALL(*conn, write(_, _, _)).WillByDefault(Arg2CallbackWithArg(5));

    ON_CALL(*idmgr, getKeyPair()).WillByDefault(ReturnRef(local_keypair));
    ON_CALL(*idmgr, getId()).WillByDefault(ReturnRef(local_pid));
    ON_CALL(*marshaller, marshal(_))
        .WillByDefault(Return(std::vector<uint8_t>(64, 1)));
  }

  std::shared_ptr<NiceMock<RawConnectionMock>> conn =
      std::make_shared<NiceMock<RawConnectionMock>>();

  constexpr static size_t ED25519_PUB_KEY_SIZE = 32;
  PublicKey remote_pubkey{
      {Key::Type::ED25519, std::vector<uint8_t>(ED25519_PUB_KEY_SIZE, 1)}};
  KeyPair local_keypair{
      {{Key::Type::ED25519, std::vector<uint8_t>(ED25519_PUB_KEY_SIZE, 2)}},
      {{Key::Type::ED25519, std::vector<uint8_t>(ED25519_PUB_KEY_SIZE, 3)}},
  };
  PeerId local_pid{PeerId::fromPublicKey(local_keypair.publicKey)};
  PeerId remote_pid{PeerId::fromPublicKey(remote_pubkey)};
};

/**
 * @given plaintext security adaptor
 * @when getting id of the underlying security protocol
 * @then an expected id is returned
 */
TEST_F(PlaintextAdaptorTest, GetId) {
  ASSERT_EQ(adaptor->getProtocolId(), "/plaintext/2.0.0");
}

/**
 * @given plaintext security adaptor
 * @when securing a raw connection inbound, using that adaptor
 * @then connection is secured
 */
TEST_F(PlaintextAdaptorTest, SecureInbound) {
  ON_CALL(*conn, close()).WillByDefault(Return(outcome::success()));
  ON_CALL(*conn, remoteMultiaddr())
      .WillByDefault(Return(libp2p::multi::Multiaddress::create(
          "/ip4/127.0.0.1/ipfs/"
          "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/")));
  plaintext::ExchangeMessage remote_msg{remote_pubkey, remote_pid};
  EXPECT_CALL(*marshaller, unmarshal(_)).WillOnce(Return(remote_msg));

  adaptor->secureInbound(
      conn, [this](outcome::result<std::shared_ptr<SecureConnection>> rc) {
        EXPECT_OUTCOME_TRUE(sec, rc);

        EXPECT_OUTCOME_TRUE(sec_remote_pubkey, sec->remotePublicKey());
        EXPECT_EQ(sec_remote_pubkey, remote_pubkey);

        EXPECT_OUTCOME_TRUE(remote_id, sec->remotePeer());
        auto calculated = PeerId::fromPublicKey(remote_pubkey);

        EXPECT_EQ(remote_id, calculated);
      });
}

/**
 * @given plaintext security adaptor
 * @when securing a raw connection outbound, using that adaptor
 * @then connection is secured
 */
TEST_F(PlaintextAdaptorTest, SecureOutbound) {
  const PeerId pid = PeerId::fromPublicKey(remote_pubkey);
  ON_CALL(*conn, close()).WillByDefault(Return(outcome::success()));
  ON_CALL(*conn, remoteMultiaddr())
      .WillByDefault(Return(libp2p::multi::Multiaddress::create(
          "/ip4/127.0.0.1/ipfs/"
          "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/")));
  plaintext::ExchangeMessage remote_msg{.pubkey = remote_pubkey,
                                        .peer_id = remote_pid};
  EXPECT_CALL(*marshaller, unmarshal(_)).WillOnce(Return(remote_msg));

  adaptor->secureOutbound(
      conn,
      pid,
      [pid, this](outcome::result<std::shared_ptr<SecureConnection>> rc) {
        EXPECT_OUTCOME_TRUE(sec, rc);

        EXPECT_OUTCOME_TRUE(sec_remote_pubkey, sec->remotePublicKey());
        EXPECT_EQ(sec_remote_pubkey, remote_pubkey);

        EXPECT_OUTCOME_TRUE(remote_id, sec->remotePeer());
        auto calculated = PeerId::fromPublicKey(remote_pubkey);

        EXPECT_EQ(remote_id, calculated);
        EXPECT_EQ(remote_id, pid);
      });
}
