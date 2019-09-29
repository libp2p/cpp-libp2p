/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect.hpp"

#include <gtest/gtest.h>
#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/tcp.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/raw_connection_mock.hpp"
#include "mock/libp2p/transport/upgrader_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/literals.hpp"
#include "testutil/ma_generator.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using libp2p::basic::ReadWriteCloser;
using libp2p::connection::CapableConnBasedOnRawConnMock;
using libp2p::connection::CapableConnection;
using libp2p::connection::RawConnection;
using libp2p::connection::RawConnectionMock;
using libp2p::connection::SecureConnection;
using libp2p::multi::Multiaddress;
using libp2p::peer::Protocol;
using libp2p::protocol_muxer::MessageManager;
using libp2p::protocol_muxer::Multiselect;
using libp2p::transport::TcpTransport;
using libp2p::transport::Upgrader;
using libp2p::transport::UpgraderMock;
using testutil::MultiaddressGenerator;

using ::testing::_;

class MultiselectTest : public ::testing::Test {
 public:
  void SetUp() override {
    context_ = std::make_shared<boost::asio::io_context>();
    upgrader = std::make_shared<UpgraderMock>();
    transport_ = std::make_shared<TcpTransport>(context_, upgrader);

    ASSERT_TRUE(transport_) << "cannot create transport";

    EXPECT_CALL(*upgrader, upgradeToSecureOutbound(_, _, _))
        .WillRepeatedly(UpgradeToSecureOutbound(
            [](auto &&raw) -> std::shared_ptr<SecureConnection> {
              return std::make_shared<CapableConnBasedOnRawConnMock>(
                  std::forward<decltype(raw)>(raw));
            }));
    EXPECT_CALL(*upgrader, upgradeToSecureInbound(_, _))
        .WillRepeatedly(UpgradeToSecureInbound(
            [](auto &&raw) -> std::shared_ptr<SecureConnection> {
              return std::make_shared<CapableConnBasedOnRawConnMock>(raw);
            }));
    EXPECT_CALL(*upgrader, upgradeToMuxed(_, _))
        .WillRepeatedly(UpgradeToMuxed(
            [](auto &&sec) -> std::shared_ptr<CapableConnection> {
              return std::make_shared<CapableConnBasedOnRawConnMock>(sec);
            }));
  }

  void TearDown() override {
    ::testing::Mock::VerifyAndClearExpectations(upgrader.get());
    transport_.reset();
    context_.reset();
    upgrader.reset();
  }

  static MultiaddressGenerator &getMaGenerator() {
    static MultiaddressGenerator ma_generator_("/ip4/127.0.0.1/tcp/", 40009);
    return ma_generator_;
  }

  std::shared_ptr<boost::asio::io_context> context_;
  std::shared_ptr<libp2p::transport::TransportAdaptor> transport_;

  std::shared_ptr<UpgraderMock> upgrader;

  const Protocol kDefaultEncryptionProtocol1 = "/plaintext/1.0.0";
  const Protocol kDefaultEncryptionProtocol2 = "/plaintext/2.0.0";

  std::vector<Protocol> protocols_{kDefaultEncryptionProtocol1,
                                   kDefaultEncryptionProtocol2};

  std::shared_ptr<Multiselect> multiselect_ = std::make_shared<Multiselect>();

  void launchContext() {
    using std::chrono_literals::operator""ms;
    context_->run_for(200ms);
  }

  /**
   * Exchange opening messages as an initiator
   */
  static void negotiationOpeningsInitiator(
      const std::shared_ptr<ReadWriteCloser> &conn,
      const std::function<void()> &next_step) {
    auto expected_opening_msg = MessageManager::openingMsg();

    auto read_msg = std::make_shared<Buffer>(expected_opening_msg.size(), 0);

    conn->read(*read_msg,
               read_msg->size(),
               [conn, read_msg, expected_opening_msg, next_step](
                   const outcome::result<size_t> &read_bytes) {
                 EXPECT_TRUE(read_bytes) << read_bytes.error().message();
                 EXPECT_EQ(*read_msg, expected_opening_msg.toVector());

                 auto write_msg =
                     std::make_shared<Buffer>(0, expected_opening_msg.size());

                 conn->write(
                     expected_opening_msg,
                     expected_opening_msg.size(),
                     [conn, expected_opening_msg, next_step](
                         const outcome::result<size_t> &written_bytes_res) {
                       EXPECT_OUTCOME_TRUE(written_bytes, written_bytes_res)
                       EXPECT_EQ(written_bytes, expected_opening_msg.size());

                       next_step();
                     });
               });
  }

  /**
   * Exchange opening messages as a listener
   * @param conn
   */
  static void negotiationOpeningsListener(
      const std::shared_ptr<ReadWriteCloser> &conn,
      const std::function<void()> &next_step) {
    auto expected_opening_msg = MessageManager::openingMsg();

    conn->write(expected_opening_msg,
                expected_opening_msg.size(),
                [conn, expected_opening_msg, next_step](
                    const outcome::result<size_t> &written_bytes_res) {
                  EXPECT_OUTCOME_TRUE(written_bytes, written_bytes_res)
                  ASSERT_EQ(written_bytes, expected_opening_msg.size());

                  auto read_msg =
                      std::make_shared<Buffer>(expected_opening_msg.size(), 0);
                  conn->read(
                      *read_msg,
                      expected_opening_msg.size(),
                      [read_msg, expected_opening_msg, next_step](
                          const outcome::result<size_t> &read_bytes_res) {
                        EXPECT_OUTCOME_TRUE(read_bytes, read_bytes_res);
                        EXPECT_EQ(read_bytes, expected_opening_msg.size());

                        EXPECT_EQ(*read_msg, expected_opening_msg);
                        next_step();
                      });
                });
  }

  /**
   * Expect to receive an LS and respond with a list of protocols
   */
  static void negotiationLsInitiator(
      const std::shared_ptr<ReadWriteCloser> &conn,
      gsl::span<const Protocol> protos_to_send,
      const std::function<void()> &next_step) {
    auto expected_ls_msg = MessageManager::lsMsg();
    auto protocols_msg = MessageManager::protocolsMsg(protos_to_send);

    auto read_msg = std::make_shared<Buffer>(expected_ls_msg.size(), 0);
    conn->read(
        *read_msg,
        expected_ls_msg.size(),
        [conn, read_msg, expected_ls_msg, protocols_msg, next_step](
            const outcome::result<size_t> &read_bytes_res) {
          EXPECT_TRUE(read_bytes_res) << read_bytes_res.error().message();
          EXPECT_OUTCOME_TRUE(read_bytes, read_bytes_res)
          EXPECT_EQ(read_bytes, expected_ls_msg.size());

          EXPECT_EQ(*read_msg, expected_ls_msg);

          conn->write(protocols_msg,
                      protocols_msg.size(),
                      [conn, protocols_msg, next_step](
                          const outcome::result<size_t> &written_bytes_res) {
                        EXPECT_OUTCOME_TRUE(written_bytes, written_bytes_res)
                        ASSERT_EQ(written_bytes, protocols_msg.size());

                        next_step();
                      });
        });
  }

  static void negotiationLsListener(
      const std::shared_ptr<ReadWriteCloser> &conn,
      gsl::span<const Protocol> protos_to_receive,
      const std::function<void()> &next_step) {
    auto ls_msg = MessageManager::lsMsg();
    auto protocols_msg = MessageManager::protocolsMsg(protos_to_receive);

    conn->write(
        ls_msg,
        ls_msg.size(),
        [conn, ls_msg, protocols_msg, next_step](
            const outcome::result<size_t> &written_bytes_res) {
          EXPECT_OUTCOME_TRUE(written_bytes, written_bytes_res)
          EXPECT_EQ(written_bytes, ls_msg.size());

          auto read_msg = std::make_shared<Buffer>(protocols_msg.size(), 0);
          conn->read(*read_msg,
                     read_msg->size(),
                     [conn, read_msg, protocols_msg, next_step](
                         const outcome::result<size_t> &read_bytes_res) {
                       EXPECT_TRUE(read_bytes_res);
                       EXPECT_EQ(*read_msg, protocols_msg);

                       next_step();
                     });
        });
  }

  /**
   * Send a protocol and expect NA as a response
   */
  static void negotiationProtocolNaListener(
      const std::shared_ptr<ReadWriteCloser> &conn,
      const Protocol &proto_to_send,
      const std::function<void()> &next_step) {
    auto na_msg = MessageManager::naMsg();
    auto protocol_msg = MessageManager::protocolMsg(proto_to_send);

    conn->write(protocol_msg,
                protocol_msg.size(),
                [conn, protocol_msg, na_msg, next_step](
                    const outcome::result<size_t> written_bytes_res) {
                  EXPECT_OUTCOME_TRUE(written_bytes, written_bytes_res)
                  EXPECT_EQ(written_bytes, protocol_msg.size());

                  auto read_msg = std::make_shared<Buffer>(na_msg.size(), 0);
                  conn->read(*read_msg,
                             read_msg->size(),
                             [conn, read_msg, na_msg, next_step](
                                 const outcome::result<size_t> read_bytes_res) {
                               EXPECT_TRUE(read_bytes_res);
                               EXPECT_EQ(*read_msg, na_msg);

                               next_step();
                             });
                });
  }

  /**
   * Receive a protocol msg and respond with the same message as an
   * acknowledgement
   */
  static void negotiationProtocolsInitiator(
      const std::shared_ptr<ReadWriteCloser> &conn,
      const Protocol &expected_protocol) {
    auto expected_proto_msg = MessageManager::protocolMsg(expected_protocol);

    auto read_msg = std::make_shared<Buffer>(expected_proto_msg.size(), 0);
    conn->read(
        *read_msg,
        expected_proto_msg.size(),
        [conn, read_msg, expected_proto_msg](
            const outcome::result<size_t> &read_bytes_res) {
          EXPECT_TRUE(read_bytes_res);
          EXPECT_EQ(*read_msg, expected_proto_msg);

          conn->write(
              *read_msg,
              read_msg->size(),
              [read_msg](const outcome::result<size_t> &written_bytes_res) {
                EXPECT_OUTCOME_TRUE(written_bytes, written_bytes_res);
                EXPECT_EQ(written_bytes, read_msg->size());
              });
        });
  }

  /**
   * Send a protocol and expect it to be received as an ack
   */
  static void negotiationProtocolsListener(
      const std::shared_ptr<ReadWriteCloser> &conn,
      const Protocol &expected_protocol) {
    auto expected_proto_msg = MessageManager::protocolMsg(expected_protocol);

    conn->write(
        expected_proto_msg,
        expected_proto_msg.size(),
        [conn,
         expected_proto_msg](const outcome::result<size_t> &written_bytes_res) {
          EXPECT_OUTCOME_TRUE(written_bytes, written_bytes_res)
          EXPECT_EQ(written_bytes, expected_proto_msg.size());

          auto read_msg =
              std::make_shared<Buffer>(expected_proto_msg.size(), 0);
          conn->read(*read_msg,
                     read_msg->size(),
                     [conn, read_msg, expected_proto_msg](
                         const outcome::result<size_t> &read_bytes_res) {
                       EXPECT_TRUE(read_bytes_res)
                           << read_bytes_res.error().message();
                       EXPECT_EQ(*read_msg, expected_proto_msg);
                     });
        });
  }
};

/**
 * @given connection, over which we want to negotiate @and multiselect instance
 * over that connection @and protocol, supported by both sides
 * @when negotiating about the protocol as an initiator side
 * @then the common protocol is selected
 */
TEST_F(MultiselectTest, NegotiateAsInitiator) {
  auto negotiated = false;
  auto transport_listener = transport_->createListener(
      [this](outcome::result<std::shared_ptr<CapableConnection>> rconn) {
        ASSERT_TRUE(rconn) << rconn.error().message();
        EXPECT_OUTCOME_TRUE(conn, rconn);
        // first, we expect an exchange of opening messages
        negotiationOpeningsInitiator(conn, [this, conn] {
          // second, ls message will be sent to us; respond with a list of
          // encryption protocols we know
          negotiationLsInitiator(conn, protocols_, [this, conn] {
            // finally, we expect that the second of the protocols will
            // be sent back to us, as it is the common one; after that,
            // we should send an ack
            negotiationProtocolsInitiator(conn, kDefaultEncryptionProtocol2);
          });
        });
      });

  auto ma = getMaGenerator().nextMultiaddress();

  ASSERT_TRUE(transport_listener->listen(ma)) << "is port busy?";
  ASSERT_TRUE(transport_->canDial(ma));

  std::vector<Protocol> protocol_vec{kDefaultEncryptionProtocol2};
  transport_->dial(
      testutil::randomPeerId(),
      ma,
      [this, &negotiated, &protocol_vec](
          outcome::result<std::shared_ptr<CapableConnection>> rconn) {
        EXPECT_OUTCOME_TRUE(conn, rconn);

        multiselect_->selectOneOf(
            protocol_vec,
            conn,
            true,
            [this, &negotiated, conn](
                const outcome::result<Protocol> &protocol_res) {
              EXPECT_OUTCOME_TRUE(protocol, protocol_res);
              EXPECT_EQ(protocol, kDefaultEncryptionProtocol2);
              negotiated = true;
            });
      });

  launchContext();
  EXPECT_TRUE(negotiated);
}

TEST_F(MultiselectTest, NegotiateAsListener) {
  auto negotiated = false;

  std::vector<Protocol> protocol_vec{kDefaultEncryptionProtocol2};
  auto transport_listener = transport_->createListener(
      [this, &negotiated, &protocol_vec](
          outcome::result<std::shared_ptr<CapableConnection>> rconn) mutable {
        EXPECT_OUTCOME_TRUE(conn, rconn);
        multiselect_->selectOneOf(
            protocol_vec,
            conn,
            false,
            [this, &negotiated](const outcome::result<Protocol> &protocol_res) {
              EXPECT_OUTCOME_TRUE(protocol, protocol_res);
              EXPECT_EQ(protocol, kDefaultEncryptionProtocol2);
              negotiated = true;
            });
      });

  auto ma = getMaGenerator().nextMultiaddress();
  ASSERT_TRUE(transport_listener->listen(ma)) << "is port busy?";
  ASSERT_TRUE(transport_->canDial(ma));

  transport_->dial(
      testutil::randomPeerId(),
      ma,
      [this](outcome::result<std::shared_ptr<CapableConnection>> rconn) {
        EXPECT_OUTCOME_TRUE(conn, rconn);
        // first, we expect an exchange of opening messages
        negotiationOpeningsListener(conn, [this, conn] {
          // second, send a protocol not supported by the other side and receive
          // an NA msg
          negotiationProtocolNaListener(
              conn, kDefaultEncryptionProtocol1, [this, conn] {
                // third, send ls and receive protocols, supported by the other
                // side
                negotiationLsListener(
                    conn,
                    std::vector<Protocol>{kDefaultEncryptionProtocol2},
                    [this, conn] {
                      // fourth, send this protocol as our choice and receive an
                      // ack
                      negotiationProtocolsListener(conn,
                                                   kDefaultEncryptionProtocol2);
                    });
              });
        });
      });

  launchContext();
  EXPECT_TRUE(negotiated);
}

/**
 * @given connection, over which we want to negotiate @and multiselect instance
 * over that connection @and encryption protocol, not supported by our side
 * @when negotiating about the protocol
 * @then the common protocol is not selected
 */
TEST_F(MultiselectTest, NegotiateFailure) {
  auto negotiated = false;

  std::vector<Protocol> protocol_vec{kDefaultEncryptionProtocol1};
  auto transport_listener = transport_->createListener(
      [this, &negotiated, &protocol_vec](
          outcome::result<std::shared_ptr<CapableConnection>> rconn) mutable {
        EXPECT_OUTCOME_TRUE(conn, rconn);

        multiselect_->selectOneOf(
            protocol_vec,
            conn,
            true,
            [](const outcome::result<Protocol> &protocol_result) {
              EXPECT_FALSE(protocol_result);
            });
        negotiated = true;
      });

  auto ma = getMaGenerator().nextMultiaddress();
  ASSERT_TRUE(transport_listener->listen(ma)) << "is port busy?";
  ASSERT_TRUE(transport_->canDial(ma));

  transport_->dial(
      testutil::randomPeerId(),
      ma,
      [this](outcome::result<std::shared_ptr<CapableConnection>> rconn) {
        EXPECT_OUTCOME_TRUE(conn, rconn);
        negotiationOpeningsInitiator(conn, [this, conn] {
          negotiationLsInitiator(
              conn, std::vector<Protocol>{kDefaultEncryptionProtocol2}, [] {});
        });
      });

  launchContext();
  ASSERT_TRUE(negotiated);
}

/**
 * @given connection, over which we want to negotiate @and multiselect instance
 * over that connection @and no protocols, supported by our side
 * @when negotiating about the protocol
 * @then the common protocol is not selected
 */
TEST_F(MultiselectTest, NoProtocols) {
  std::shared_ptr<RawConnection> conn = std::make_shared<RawConnectionMock>();
  std::vector<Protocol> empty_vec{};
  multiselect_->selectOneOf(
      empty_vec, conn, true, [](const outcome::result<Protocol> &protocol_res) {
        EXPECT_FALSE(protocol_res);
      });
}
