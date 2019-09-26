/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamuxed_connection.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/muxer/yamux/yamux_frame.hpp"
#include "libp2p/muxer/yamux/yamux_stream.hpp"
#include "libp2p/transport/tcp.hpp"
#include "libp2p/transport/upgrader.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/transport/upgrader_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::connection;
using namespace libp2p::transport;
using namespace kagome::common;
using namespace libp2p::multi;
using namespace libp2p::basic;
using ::testing::_;

class YamuxIntegrationTest : public testing::Test {
 public:
  void SetUp() override {
    context_ = std::make_shared<boost::asio::io_context>();
    transport_ = std::make_shared<TcpTransport>(context_, upgrader);
    ASSERT_TRUE(transport_) << "cannot create transport";

    EXPECT_CALL(*upgrader, upgradeToSecureOutbound(_, _, _))
        .WillRepeatedly(UpgradeToSecureOutbound(
            [](auto &&raw) -> std::shared_ptr<SecureConnection> {
              return std::make_shared<CapableConnBasedOnRawConnMock>(raw);
              ;
            }));
    EXPECT_CALL(*upgrader, upgradeToSecureInbound(_, _))
        .WillRepeatedly(UpgradeToSecureInbound(
            [](auto &&raw) -> std::shared_ptr<SecureConnection> {
              return std::make_shared<CapableConnBasedOnRawConnMock>(raw);
            }));
    EXPECT_CALL(*upgrader, upgradeToMuxed(_, _))
        .WillRepeatedly(UpgradeToMuxed(
            [](auto &&sec) -> std::shared_ptr<CapableConnection> {
              return std::make_shared<YamuxedConnection>(sec);
            }));

    auto ma = "/ip4/127.0.0.1/tcp/40009"_multiaddr;
    multiaddress_ = std::make_shared<Multiaddress>(std::move(ma));

    // setup a server, which is going to remember all incoming streams
    transport_listener_ =
        transport_->createListener([this](auto &&conn_res) mutable {
          EXPECT_OUTCOME_TRUE(conn, conn_res)

          yamuxed_connection_ =
              std::move(std::static_pointer_cast<YamuxedConnection>(conn));
          yamuxed_connection_->onStream([this](auto &&stream) {
            ASSERT_TRUE(stream);
            accepted_streams_.push_back(std::forward<decltype(stream)>(stream));
          });
          yamuxed_connection_->start();
          invokeCallbacks();
        });
    ASSERT_TRUE(transport_listener_->listen(*multiaddress_))
        << "is port 40009 busy?";
  }

  void launchContext() {
    using std::chrono_literals::operator""ms;
    context_->run_for(200ms);
  }

  /**
   * Add a callback, which is called, when the connection is dialed and yamuxed
   * @param cb to be added
   */
  void withYamuxedConn(
      std::function<void(std::shared_ptr<YamuxedConnection>)> cb) {
    if (yamuxed_connection_) {
      return cb(yamuxed_connection_);
    }
    yamux_callbacks_.push_back(std::move(cb));
  }

  /**
   * Invoke all callbacks, which were waiting for the connection to be yamuxed
   */
  void invokeCallbacks() {
    std::for_each(yamux_callbacks_.begin(),
                  yamux_callbacks_.end(),
                  [this](const auto &cb) { cb(yamuxed_connection_); });
    yamux_callbacks_.clear();
  }

  /**
   * Get a pointer to a new stream
   * @param expected_stream_id - id, which is expected to be assigned to that
   * stream
   * @return pointer to the stream
   * @note the caller must ensure yamuxed_connection_ existsd before calling
   */
  void withStream(std::shared_ptr<ReadWriteCloser> conn,
                  std::function<void(std::shared_ptr<Stream>)> cb,
                  YamuxedConnection::StreamId expected_stream_id =
                      kDefaulExpectedStreamId) {
    auto new_stream_msg =
        std::make_shared<Buffer>(newStreamMsg(expected_stream_id));
    auto rcvd_msg = std::make_shared<Buffer>(new_stream_msg->size(), 0);

    yamuxed_connection_->newStream(
        [c = std::move(conn), cb = std::move(cb), new_stream_msg, rcvd_msg](
            auto &&stream_res) mutable {
          ASSERT_TRUE(stream_res);
          c->read(*rcvd_msg,
                  new_stream_msg->size(),
                  [c,
                   stream = std::move(stream_res.value()),
                   new_stream_msg,
                   rcvd_msg,
                   cb = std::move(cb)](auto &&res) {
                    ASSERT_TRUE(res);
                    ASSERT_EQ(*rcvd_msg, *new_stream_msg);
                    cb(std::move(stream));
                  });
        });
  }

  std::shared_ptr<boost::asio::io_context> context_;
  std::shared_ptr<libp2p::transport::TransportAdaptor> transport_;
  std::shared_ptr<libp2p::transport::TransportListener> transport_listener_;
  std::shared_ptr<libp2p::multi::Multiaddress> multiaddress_;

  std::shared_ptr<YamuxedConnection> yamuxed_connection_;
  std::vector<std::shared_ptr<Stream>> accepted_streams_;

  std::shared_ptr<UpgraderMock> upgrader = std::make_shared<UpgraderMock>();

  std::vector<std::function<void(std::shared_ptr<YamuxedConnection>)>>
      yamux_callbacks_;

  bool client_finished_ = false;

  static constexpr YamuxedConnection::StreamId kDefaulExpectedStreamId = 2;
};

/**
 * @given initialized Yamux
 * @when creating a new stream from the client's side
 * @then stream is created @and corresponding ack message is sent to the client
 */
TEST_F(YamuxIntegrationTest, StreamFromClient) {
  constexpr YamuxedConnection::StreamId created_stream_id = 1;

  auto new_stream_ack_msg_rcv =
      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);
  auto new_stream_msg = newStreamMsg(created_stream_id);

  transport_->dial(
      testutil::randomPeerId(),
      *multiaddress_,
      [this, created_stream_id, &new_stream_msg, new_stream_ack_msg_rcv](
          auto &&conn_res) {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        // downcast the connection, as direct writes to capables are forbidden
        conn->write(
            new_stream_msg,
            new_stream_msg.size(),
            [this, conn, created_stream_id, new_stream_ack_msg_rcv](
                auto &&res) {
              ASSERT_TRUE(res) << res.error().message();
              conn->read(
                  *new_stream_ack_msg_rcv,
                  YamuxFrame::kHeaderLength,
                  [this, created_stream_id, new_stream_ack_msg_rcv, conn](
                      auto &&res) {
                    ASSERT_TRUE(res);

                    // check a new stream is in our 'accepted_streams'
                    ASSERT_EQ(accepted_streams_.size(), 1);

                    // check our yamux has sent an ack message for that
                    // stream
                    auto parsed_ack_opt =
                        parseFrame(new_stream_ack_msg_rcv->toVector());
                    ASSERT_TRUE(parsed_ack_opt);
                    ASSERT_EQ(parsed_ack_opt->stream_id, created_stream_id);

                    client_finished_ = true;
                  });
            });
      });

  launchContext();
  ASSERT_TRUE(client_finished_);
}

/**
 * @given initialized Yamux
 * @when creating a new stream from the server's side
 * @then stream is created @and corresponding new stream message is received by
 * the client
 */
TEST_F(YamuxIntegrationTest, StreamFromServer) {
  constexpr YamuxedConnection::StreamId expected_stream_id = 2;

  auto expected_new_stream_msg = newStreamMsg(expected_stream_id);
  auto new_stream_msg_buf =
      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);

  transport_->dial(
      testutil::randomPeerId(),
      *multiaddress_,
      [this, &expected_new_stream_msg, new_stream_msg_buf](auto &&conn_res) {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        withYamuxedConn(
            [this, conn, &expected_new_stream_msg, new_stream_msg_buf](
                auto &&yamuxed_conn) {
              yamuxed_conn->newStream([this,
                                       conn,
                                       &expected_new_stream_msg,
                                       new_stream_msg_buf](auto &&stream_res) {
                EXPECT_OUTCOME_TRUE(stream, stream_res)
                ASSERT_FALSE(stream->isClosedForRead());
                ASSERT_FALSE(stream->isClosedForWrite());
                ASSERT_FALSE(stream->isClosed());

                conn->read(
                    *new_stream_msg_buf,
                    new_stream_msg_buf->size(),
                    [this, conn, &expected_new_stream_msg, new_stream_msg_buf](
                        auto &&res) {
                      ASSERT_TRUE(res);
                      ASSERT_EQ(*new_stream_msg_buf, expected_new_stream_msg);
                      client_finished_ = true;
                    });
              });
            });
        return outcome::success();
      });

  launchContext();
  ASSERT_TRUE(client_finished_);
}

/**
 * @given initialized Yamux @and streams, multiplexed by that Yamux
 * @When writing to that stream
 * @then the operation is succesfully executed
 */
TEST_F(YamuxIntegrationTest, StreamWrite) {
  Buffer data{{0x12, 0x34, 0xAA}};
  auto expected_data_msg = dataMsg(kDefaulExpectedStreamId, data);
  auto received_data_msg =
      std::make_shared<Buffer>(expected_data_msg.size(), 0);

  transport_->dial(
      testutil::randomPeerId(),
      *multiaddress_,
      [this, &data, &expected_data_msg, received_data_msg](auto &&conn_res) {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        withYamuxedConn([this,
                         conn,
                         &data,
                         &expected_data_msg,
                         received_data_msg](auto &&yamuxed_conn) {
          withStream(
              conn,
              [this, conn, &data, &expected_data_msg, received_data_msg](
                  auto &&stream) {
                stream->write(
                    data,
                    data.size(),
                    [this, conn, &expected_data_msg, received_data_msg](
                        auto &&res) {
                      ASSERT_TRUE(res);
                      // check that our written data has
                      // achieved the destination
                      conn->read(
                          *received_data_msg,
                          expected_data_msg.size(),
                          [this, conn, &expected_data_msg, received_data_msg](
                              auto &&res) {
                            ASSERT_TRUE(res);
                            ASSERT_EQ(*received_data_msg, expected_data_msg);
                            client_finished_ = true;
                          });
                    });
              });
        });
        return outcome::success();
      });

  launchContext();
  ASSERT_TRUE(client_finished_);
}

/**
 * @given initialized Yamux @and streams, multiplexed by that Yamux
 * @when reading from that stream
 * @then the operation is successfully executed
 */
TEST_F(YamuxIntegrationTest, StreamRead) {
  Buffer data{{0x12, 0x34, 0xAA}};
  auto written_data_msg = dataMsg(kDefaulExpectedStreamId, data);
  auto rcvd_data_msg = std::make_shared<Buffer>(data.size(), 0);

  transport_->dial(
      testutil::randomPeerId(),
      *multiaddress_,
      [this, &data, &written_data_msg, rcvd_data_msg](auto &&conn_res) {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        withYamuxedConn([this, conn, &data, &written_data_msg, rcvd_data_msg](
                            auto &&yamuxed_conn) {
          withStream(
              conn,
              [this, conn, &data, &written_data_msg, rcvd_data_msg](
                  auto &&stream) {
                conn->write(
                    written_data_msg,
                    written_data_msg.size(),
                    [this, conn, stream, &data, rcvd_data_msg](auto &&res) {
                      ASSERT_TRUE(res);
                      stream->read(
                          *rcvd_data_msg,
                          data.size(),
                          [this, stream, &data, rcvd_data_msg](auto &&res) {
                            ASSERT_TRUE(res);
                            ASSERT_EQ(*rcvd_data_msg, data);
                            client_finished_ = true;
                          });
                    });
              });
        });
        return outcome::success();
      });

  launchContext();
  ASSERT_TRUE(client_finished_);
}

/**
 * @given initialized Yamux @and stream over it
 * @when closing that stream for writes
 * @then the stream is closed for writes @and corresponding message is
 received
 * on the other side
 */
TEST_F(YamuxIntegrationTest, CloseForWrites) {
  auto expected_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);
  auto close_stream_msg_rcv =
      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);

  transport_->dial(
      testutil::randomPeerId(),
      *multiaddress_,
      [this, &expected_close_stream_msg, close_stream_msg_rcv](
          auto &&conn_res) {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        withYamuxedConn([this,
                         conn,
                         &expected_close_stream_msg,
                         close_stream_msg_rcv](auto &&yamuxed_conn) {
          withStream(
              conn,
              [this, conn, &expected_close_stream_msg, close_stream_msg_rcv](
                  auto &&stream) {
                ASSERT_FALSE(stream->isClosedForWrite());

                stream->close([this,
                               conn,
                               stream,
                               &expected_close_stream_msg,
                               close_stream_msg_rcv](auto &&res) {
                  ASSERT_TRUE(res);
                  ASSERT_TRUE(stream->isClosedForWrite());

                  conn->read(*close_stream_msg_rcv,
                             expected_close_stream_msg.size(),
                             [this,
                              conn,
                              &expected_close_stream_msg,
                              close_stream_msg_rcv](auto &&res) {
                               ASSERT_TRUE(res);
                               ASSERT_EQ(*close_stream_msg_rcv,
                                         expected_close_stream_msg);
                               client_finished_ = true;
                             });
                });
              });
        });
        return outcome::success();
      });

  launchContext();
  ASSERT_TRUE(client_finished_);
}

/**
 * @given initialized Yamux @and stream over it
 * @when the other side sends a close message for that stream
 * @then the stream is closed for reads
 */
TEST_F(YamuxIntegrationTest, CloseForReads) {
  std::shared_ptr<Stream> ret_stream;
  auto sent_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);

  transport_->dial(
      testutil::randomPeerId(),
      *multiaddress_,
      [this, &sent_close_stream_msg, &ret_stream](auto &&conn_res) mutable {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        withYamuxedConn([this, conn, &sent_close_stream_msg, &ret_stream](
                            auto &&yamuxed_conn) mutable {
          withStream(
              conn,
              [this, conn, &sent_close_stream_msg, &ret_stream](
                  auto &&stream) mutable {
                ASSERT_FALSE(stream->isClosedForRead());
                conn->write(
                    sent_close_stream_msg,
                    sent_close_stream_msg.size(),
                    [this, conn, stream, &ret_stream](auto &&res) mutable {
                      ASSERT_TRUE(res);
                      ret_stream = std::forward<decltype(stream)>(stream);
                      client_finished_ = true;
                    });
              });
        });
        return outcome::success();
      });

  launchContext();
  ASSERT_TRUE(ret_stream->isClosedForRead());
  ASSERT_TRUE(client_finished_);
}

/**
 * @given initialized Yamux @and stream over it
 * @when close message is sent over the stream @and the other side responses
 * with a close message as well
 * @then the stream is closed entirely - removed from Yamux
 */
TEST_F(YamuxIntegrationTest, CloseEntirely) {
  std::shared_ptr<Stream> ret_stream;
  auto expected_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);
  auto close_stream_msg_rcv =
      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);

  transport_->dial(
      testutil::randomPeerId(),
      *multiaddress_,
      [this, &expected_close_stream_msg, close_stream_msg_rcv, &ret_stream](
          auto &&conn_res) mutable {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        withYamuxedConn([this,
                         conn,
                         &expected_close_stream_msg,
                         close_stream_msg_rcv,
                         &ret_stream](auto &&) mutable {
          withStream(conn,
                     [this,
                      conn,
                      &expected_close_stream_msg,
                      close_stream_msg_rcv,
                      &ret_stream](auto &&stream) mutable {
                       ASSERT_FALSE(stream->isClosed());
                       stream->close([this,
                                      conn,
                                      stream,
                                      &expected_close_stream_msg,
                                      close_stream_msg_rcv,
                                      &ret_stream](auto &&res) mutable {
                         ASSERT_TRUE(res);
                         conn->read(
                             *close_stream_msg_rcv,
                             close_stream_msg_rcv->size(),
                             [this,
                              conn,
                              stream,
                              &expected_close_stream_msg,
                              close_stream_msg_rcv,
                              &ret_stream](auto &&res) mutable {
                               ASSERT_TRUE(res);
                               ASSERT_EQ(*close_stream_msg_rcv,
                                         expected_close_stream_msg);
                               conn->write(
                                   expected_close_stream_msg,
                                   expected_close_stream_msg.size(),
                                   [this, conn, stream, &ret_stream](
                                       auto &&res) mutable {
                                     ASSERT_TRUE(res);
                                     ret_stream =
                                         std::forward<decltype(stream)>(stream);
                                     client_finished_ = true;
                                   });
                             });
                       });
                     });
        });
        return outcome::success();
      });

  launchContext();
  ASSERT_TRUE(ret_stream->isClosed());
  ASSERT_TRUE(client_finished_);
}

/**
 * @given initialized Yamux
 * @when a ping message arrives to Yamux
 * @then Yamux sends a ping response back
 */
TEST_F(YamuxIntegrationTest, Ping) {
  static constexpr uint32_t ping_value = 42;

  auto ping_in_msg = pingOutMsg(ping_value);
  auto ping_out_msg = pingResponseMsg(ping_value);
  auto received_ping = std::make_shared<Buffer>(ping_out_msg.size(), 0);

  transport_->dial(
      testutil::randomPeerId(),
      *multiaddress_,
      [this, &ping_in_msg, &ping_out_msg, received_ping](auto &&conn_res) {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        conn->write(ping_in_msg,
                    ping_in_msg.size(),
                    [this, conn, &ping_out_msg, received_ping](auto &&res) {
                      ASSERT_TRUE(res);
                      conn->read(*received_ping,
                                 received_ping->size(),
                                 [this, conn, &ping_out_msg, received_ping](
                                     auto &&res) {
                                   ASSERT_TRUE(res);
                                   ASSERT_EQ(*received_ping, ping_out_msg);
                                   client_finished_ = true;
                                 });
                    });
      });

  launchContext();
  ASSERT_TRUE(client_finished_);
}

/**
 * @given initialized Yamux @and stream over it
 * @when a reset message is sent over that stream
 * @then the stream is closed entirely - removed from Yamux @and the other
 side
 * receives a corresponding message
 */
TEST_F(YamuxIntegrationTest, Reset) {
  std::shared_ptr<Stream> ret_stream;
  auto expected_reset_msg = resetStreamMsg(kDefaulExpectedStreamId);
  auto rcvd_msg = std::make_shared<Buffer>(expected_reset_msg.size(), 0);

  transport_->dial(
      testutil::randomPeerId(),
      *multiaddress_,
      [this, &ret_stream, &expected_reset_msg, rcvd_msg](
          auto &&conn_res) mutable {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        withYamuxedConn(
            [this, conn, &ret_stream, &expected_reset_msg, rcvd_msg](
                auto &&) mutable {
              withStream(
                  conn,
                  [this, conn, &ret_stream, &expected_reset_msg, rcvd_msg](
                      auto &&stream) mutable {
                    ASSERT_FALSE(stream->isClosed());
                    stream->reset();
                    conn->read(*rcvd_msg,
                               expected_reset_msg.size(),
                               [this,
                                conn,
                                &ret_stream,
                                stream,
                                &expected_reset_msg,
                                rcvd_msg](auto &&res) mutable {
                                 ASSERT_TRUE(res);
                                 ASSERT_EQ(*rcvd_msg, expected_reset_msg);
                                 ret_stream =
                                     std::forward<decltype(stream)>(stream);
                                 client_finished_ = true;
                               });
                  });
            });
        return outcome::success();
      });

  launchContext();
  ASSERT_TRUE(client_finished_);
  ASSERT_TRUE(ret_stream->isClosed());
}
