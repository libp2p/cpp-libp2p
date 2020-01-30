/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <random>

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/peer/impl/identity_manager_impl.hpp>
#include <libp2p/security/plaintext.hpp>
#include <libp2p/security/plaintext/exchange_message_marshaller_impl.hpp>
#include <libp2p/transport/tcp.hpp>
#include <mock/libp2p/crypto/key_validator_mock.hpp>
#include <mock/libp2p/transport/upgrader_mock.hpp>
#include "testutil/libp2p/peer.hpp"
#include "testutil/outcome.hpp"

/// tested muxers
#include <libp2p/muxer/mplex.hpp>
#include <libp2p/muxer/yamux.hpp>

using namespace libp2p;
using namespace transport;
using namespace connection;
using namespace muxer;
using namespace security;
using namespace multi;
using namespace peer;
using namespace crypto;
using namespace marshaller;
using namespace validator;
using namespace libp2p::common;

using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
using std::chrono_literals::operator""ms;

static const size_t kServerBufSize = 10000;  // 10 Kb

// verbose==true will allow to print debug output to stdout
static const bool verbose = false;

struct UpgraderSemiMock : public Upgrader {
  ~UpgraderSemiMock() override = default;

  UpgraderSemiMock(std::shared_ptr<SecurityAdaptor> s,
                   std::shared_ptr<MuxerAdaptor> m)
      : security(std::move(s)), mux(std::move(m)) {}

  void upgradeToSecureOutbound(RawSPtr conn, const peer::PeerId &remoteId,
                               OnSecuredCallbackFunc cb) override {
    security->secureOutbound(conn, remoteId, std::move(cb));
  }

  void upgradeToSecureInbound(RawSPtr conn, OnSecuredCallbackFunc cb) override {
    security->secureInbound(conn, std::move(cb));
  }

  void upgradeToMuxed(SecSPtr conn, OnMuxedCallbackFunc cb) override {
    mux->muxConnection(std::move(conn), [cb = std::move(cb)](auto &&conn_res) {
      EXPECT_OUTCOME_TRUE(conn, conn_res)
      conn->start();
      cb(std::move(conn));
    });
  }

  std::shared_ptr<SecurityAdaptor> security;
  std::shared_ptr<MuxerAdaptor> mux;
};

struct Server : public std::enable_shared_from_this<Server> {
  explicit Server(std::shared_ptr<TransportAdaptor> transport)
      : transport_(std::move(transport)) {}

  void onConnection(const std::shared_ptr<CapableConnection> &conn) {
    this->clientsConnected++;

    conn->onStream([this](outcome::result<std::shared_ptr<Stream>> rstream) {
      EXPECT_OUTCOME_TRUE(stream, rstream)
      this->println("new stream created");
      this->streamsCreated++;
      auto buf = std::make_shared<std::vector<uint8_t>>();
      this->onStream(buf, stream);
    });
  }

  void onStream(const std::shared_ptr<std::vector<uint8_t>> &buf,
                const std::shared_ptr<Stream> &stream) {
    // we should create buffer per stream (session)
    buf->resize(kServerBufSize);

    println("onStream executed");

    stream->readSome(
        *buf, buf->size(), [buf, stream, this](outcome::result<size_t> rread) {
          if (!rread) {
            if (rread.error() == YamuxedConnection::Error::CLOSED_BY_PEER) {
              // TODO(artem): EOF wasn't signalled by former logic,
              // check if this complies to this test's intentions
              return;
            }
            this->println("readSome error: ", rread.error().message());
          }

          EXPECT_OUTCOME_TRUE(read, rread)

          this->println("readSome ", read, " bytes");
          this->streamReads++;

          // 01-echo back read data
          stream->write(
              *buf, read,
              [buf, read, stream, this](outcome::result<size_t> rwrite) {
                EXPECT_OUTCOME_TRUE(write, rwrite)
                this->println("write ", write, " bytes");
                this->streamWrites++;
                ASSERT_EQ(write, read);

                this->onStream(buf, stream);
              });
        });
  }

  void listen(const Multiaddress &ma) {
    listener_ = transport_->createListener(
        [this](outcome::result<std::shared_ptr<CapableConnection>> rconn) {
          EXPECT_OUTCOME_TRUE(conn, rconn)
          this->println("new connection received");
          this->onConnection(conn);
        });

    EXPECT_OUTCOME_TRUE_1(this->listener_->listen(ma));
  }

  size_t clientsConnected = 0;
  size_t streamsCreated = 0;
  size_t streamReads = 0;
  size_t streamWrites = 0;

 private:
  template <typename... Args>
  void println(Args &&... args) {
    if (!verbose) return;
    std::cout << "[server " << std::this_thread::get_id() << "]\t";
    (std::cout << ... << args);
    std::cout << std::endl;
  }

  std::shared_ptr<TransportAdaptor> transport_;
  std::shared_ptr<TransportListener> listener_;
};

struct Client : public std::enable_shared_from_this<Client> {
  Client(std::shared_ptr<TcpTransport> transport, size_t seed,
         std::shared_ptr<boost::asio::io_context> context, size_t streams,
         size_t rounds)
      : context_(context),
        streams_(streams),
        rounds_(rounds),
        generator(seed),
        distribution(1, kServerBufSize),
        transport_(std::move(transport)) {}

  void connect(const PeerId &p, const Multiaddress &server) {
    // create new stream
    transport_->dial(
        p, server,
        [this](outcome::result<std::shared_ptr<CapableConnection>> rconn) {
          EXPECT_OUTCOME_TRUE(conn, rconn);
          this->println("connected");
          this->onConnection(conn);
        });
  }

  void onConnection(const std::shared_ptr<CapableConnection> &conn) {
    for (size_t i = 0; i < streams_; i++) {
      boost::asio::post(*context_, [i, conn, this]() {
        conn->newStream(
            [i, conn, this](outcome::result<std::shared_ptr<Stream>> rstream) {
              EXPECT_OUTCOME_TRUE(stream, rstream);
              this->println("new stream number ", i, " created");
              this->onStream(i, this->rounds_, stream);
            });
      });
    }
  }

  void onStream(size_t streamId, size_t round,
                const std::shared_ptr<Stream> &stream) {
    this->println(streamId, " onStream round ", round);
    if (round <= 0) {
      return;
    }

    auto buf = randomBuffer();
    stream->write(
        *buf, buf->size(),
        [round, streamId, buf, stream, this](outcome::result<size_t> rwrite) {
          EXPECT_OUTCOME_TRUE(write, rwrite);
          this->println(streamId, " write ", write, " bytes");
          this->streamWrites++;

          auto readbuf = std::make_shared<std::vector<uint8_t>>();
          readbuf->resize(write);

          stream->readSome(*readbuf, readbuf->size(),
                           [round, streamId, write, buf, readbuf, stream,
                            this](outcome::result<size_t> rread) {
                             EXPECT_OUTCOME_TRUE(read, rread);
                             this->println(streamId, " readSome ", read,
                                           " bytes");
                             this->streamReads++;

                             ASSERT_EQ(write, read);
                             ASSERT_EQ(*buf, *readbuf);

                             this->onStream(streamId, round - 1, stream);
                           });
        });
  }

  size_t streamWrites = 0;
  size_t streamReads = 0;

 private:
  template <typename... Args>
  void println(Args &&... args) {
    if (!verbose) return;
    std::cout << "[client " << std::this_thread::get_id() << "]\t";
    (std::cout << ... << args);
    std::cout << std::endl;
  }

  size_t rand() {
    return distribution(generator);
  }

  std::shared_ptr<std::vector<uint8_t>> randomBuffer() {
    auto buf = std::make_shared<std::vector<uint8_t>>();
    buf->resize(this->rand());
    this->println("random buffer of size ", buf->size(), " generated");
    std::generate(buf->begin(), buf->end(), [self{this->shared_from_this()}]() {
      return self->rand() & 0xff;
    });
    return buf;
  }

  std::shared_ptr<boost::asio::io_context> context_;

  size_t streams_;
  size_t rounds_;

  std::default_random_engine generator;
  std::uniform_int_distribution<int> distribution;

  std::shared_ptr<TransportAdaptor> transport_;
};

struct MuxerAcceptanceTest
    : public ::testing::TestWithParam<std::shared_ptr<MuxerAdaptor>> {
  struct PrintToStringParamName {
    template <class ParamType>
    std::string operator()(
        const testing::TestParamInfo<ParamType> &info) const {
      auto m = static_cast<std::shared_ptr<MuxerAdaptor>>(info.param);
      auto p = m->getProtocolId();

      return p.substr(1, p.find('/', 1) - 1);
    }
  };
};

TEST_P(MuxerAcceptanceTest, ParallelEcho) {
  // total number of parallel clients
  const int totalClients = 3;
  // total number of streams per connection
  const int streams = 10;
  // total number of rounds per stream
  const int rounds = 10;
  // number, which makes tests reproducible
  const int seed = 0;

  auto context = std::make_shared<boost::asio::io_context>(1);
  std::default_random_engine randomEngine(seed);

  auto serverAddr = "/ip4/127.0.0.1/tcp/40312"_multiaddr;

  KeyPair serverKeyPair = {{{Key::Type::Ed25519, {1}}},
                           {{Key::Type::Ed25519, {2}}}};

  auto key_validator = std::make_shared<KeyValidatorMock>();
  auto key_marshaller = std::make_shared<KeyMarshallerImpl>(key_validator);
  EXPECT_CALL(*key_validator, validate(::testing::An<const PrivateKey &>()))
      .WillRepeatedly(::testing::Return(outcome::success()));
  EXPECT_CALL(*key_validator, validate(::testing::An<const PublicKey &>()))
      .WillRepeatedly(::testing::Return(outcome::success()));

  auto muxer = GetParam();
  auto idmgr =
      std::make_shared<IdentityManagerImpl>(serverKeyPair, key_marshaller);
  auto msg_marshaller =
      std::make_shared<plaintext::ExchangeMessageMarshallerImpl>(
          key_marshaller);
  auto plaintext =
      std::make_shared<Plaintext>(msg_marshaller, idmgr, key_marshaller);
  auto upgrader = std::make_shared<UpgraderSemiMock>(plaintext, muxer);
  auto transport = std::make_shared<TcpTransport>(context, upgrader);
  auto server = std::make_shared<Server>(transport);
  server->listen(serverAddr);

  std::vector<std::thread> clients;
  clients.reserve(totalClients);
  for (int i = 0; i < totalClients; i++) {
    auto localSeed = randomEngine();
    clients.emplace_back([&, localSeed]() {
      auto context = std::make_shared<boost::asio::io_context>(1);

      KeyPair clientKeyPair = {{{Key::Type::Ed25519, {3}}},
                               {{Key::Type::Ed25519, {4}}}};

      auto muxer = GetParam();
      auto key_marshaller = std::make_shared<KeyMarshallerImpl>(key_validator);
      auto idmgr =
          std::make_shared<IdentityManagerImpl>(clientKeyPair, key_marshaller);
      auto msg_marshaller =
          std::make_shared<plaintext::ExchangeMessageMarshallerImpl>(
              key_marshaller);
      auto plaintext =
          std::make_shared<Plaintext>(msg_marshaller, idmgr, key_marshaller);
      auto upgrader = std::make_shared<UpgraderSemiMock>(plaintext, muxer);
      auto transport = std::make_shared<TcpTransport>(context, upgrader);
      auto client = std::make_shared<Client>(transport, localSeed, context,
                                             streams, rounds);

      EXPECT_OUTCOME_TRUE(marshalled_key,
                          key_marshaller->marshal(serverKeyPair.publicKey))
      EXPECT_OUTCOME_TRUE(p, PeerId::fromPublicKey(marshalled_key))
      client->connect(p, serverAddr);

      context->run_for(2000ms);

      EXPECT_EQ(client->streamWrites, rounds * streams);
      EXPECT_EQ(client->streamReads, rounds * streams);
    });
  }

  context->run_for(3000ms);

  for (auto &c : clients) {
    if (c.joinable()) {
      c.join();
    }
  }

  EXPECT_EQ(server->clientsConnected, totalClients);
  EXPECT_EQ(server->streamsCreated, totalClients * streams);
  EXPECT_EQ(server->streamReads, totalClients * streams * rounds);
  EXPECT_EQ(server->streamWrites, totalClients * streams * rounds);
}

INSTANTIATE_TEST_CASE_P(
    AllMuxers, MuxerAcceptanceTest,
    ::testing::Values(
        // list here all muxers
        std::make_shared<Yamux>(muxer::MuxedConnectionConfig{1048576, 1000}),
        std::make_shared<Mplex>(muxer::MuxedConnectionConfig{})),
    MuxerAcceptanceTest::PrintToStringParamName());
