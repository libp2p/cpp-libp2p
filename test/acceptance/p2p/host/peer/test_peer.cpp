/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "acceptance/libp2p/host/peer/test_peer.hpp"

#include <gtest/gtest.h>
#include "acceptance/libp2p/host/peer/tick_counter.hpp"
#include "acceptance/libp2p/host/protocol/client_test_session.hpp"
#include "libp2p/security/plaintext/exchange_message_marshaller_impl.hpp"

Peer::Peer(Peer::Duration timeout)
    : muxed_config_{1024576, 1000},
      timeout_{timeout},
      context_{std::make_shared<Context>()},
      echo_{std::make_shared<protocol::Echo>()},
      random_provider_{
          std::make_shared<crypto::random::BoostRandomGenerator>()},
      key_generator_{
          std::make_shared<crypto::KeyGeneratorImpl>(*random_provider_)} {
  EXPECT_OUTCOME_TRUE_MSG(
      keys,
      key_generator_->generateKeys(crypto::Key::Type::ED25519),
      "failed to generate keys");

  host_ = makeHost(std::move(keys));

  host_->setProtocolHandler(echo_->getProtocolId(),
                            [this](std::shared_ptr<connection::Stream> result) {
                              echo_->handle(result);
                            });
}

void Peer::startServer(const multi::Multiaddress &address,
                       std::shared_ptr<std::promise<peer::PeerInfo>> promise) {
  context_->post([this, address, p = std::move(promise)] {
    EXPECT_OUTCOME_TRUE_MSG_1(host_->listen(address), "failed to start server");
    host_->start();
    p->set_value(host_->getPeerInfo());
  });

  thread_ = std::thread([this] { context_->run_for(timeout_); });
}

void Peer::startClient(const peer::PeerInfo &pinfo,
                       size_t message_count,
                       Peer::sptr<TickCounter> counter) {
  context_->post([this,
                  server_id = pinfo.id.toBase58(),
                  pinfo,
                  message_count,
                  counter = std::move(counter)]() mutable {
    this->host_->newStream(
        pinfo,
        echo_->getProtocolId(),
        [server_id = std::move(server_id),
         ping_times = message_count,
         counter = std::move(counter)](
            outcome::result<sptr<Stream>> rstream) mutable {
          // get stream
          EXPECT_OUTCOME_TRUE_MSG(
              stream, rstream, "failed to connect to server: " + server_id);
          // make client session
          auto client =
              std::make_shared<protocol::ClientTestSession>(stream, ping_times);
          // handle session
          client->handle(
              [server_id = std::move(server_id),
               client,
               counter = std::move(counter)](
                  outcome::result<std::vector<uint8_t>> res) mutable {
                // count message exchange
                counter->tick();
                // ensure message returned
                EXPECT_OUTCOME_TRUE_MSG(
                    vec,
                    res,
                    "failed to receive response from server: " + server_id);
                // ensure message is correct
                ASSERT_EQ(vec.size(), client->bufferSize());  // NOLINT
              });
        });
  });
}

void Peer::wait() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

Peer::sptr<host::BasicHost> Peer::makeHost(crypto::KeyPair keyPair) {
  auto idmgr = std::make_shared<peer::IdentityManagerImpl>(keyPair);

  auto multiselect = std::make_shared<protocol_muxer::Multiselect>();

  auto router = std::make_shared<network::RouterImpl>();

  auto key_generator =
      std::make_shared<crypto::KeyGeneratorImpl>(*random_provider_);

  auto key_validator = std::make_shared<crypto::validator::KeyValidatorImpl>(
      std::move(key_generator));

  auto key_marshaller = std::make_shared<crypto::marshaller::KeyMarshallerImpl>(
      std::move(key_validator));

  auto exchange_msg_marshaller =
      std::make_shared<security::plaintext::ExchangeMessageMarshallerImpl>(
          std::move(key_marshaller));

  std::vector<std::shared_ptr<security::SecurityAdaptor>> security_adaptors = {
      std::make_shared<security::Plaintext>(std::move(exchange_msg_marshaller),
                                            idmgr)};

  std::vector<std::shared_ptr<muxer::MuxerAdaptor>> muxer_adaptors = {
      std::make_shared<muxer::Yamux>(muxed_config_)};

  auto upgrader = std::make_shared<transport::UpgraderImpl>(
      multiselect, std::move(security_adaptors), std::move(muxer_adaptors));

  std::vector<std::shared_ptr<transport::TransportAdaptor>> transports = {
      std::make_shared<transport::TcpTransport>(context_, std::move(upgrader))};

  auto tmgr =
      std::make_shared<network::TransportManagerImpl>(std::move(transports));

  auto cmgr = std::make_shared<network::ConnectionManagerImpl>(tmgr);

  auto listener = std::make_unique<network::ListenerManagerImpl>(
      multiselect, std::move(router), tmgr, cmgr);

  auto dialer = std::make_unique<network::DialerImpl>(multiselect, tmgr, cmgr);

  auto network = std::make_unique<network::NetworkImpl>(
      std::move(listener), std::move(dialer), cmgr);

  auto addr_repo = std::make_shared<peer::InmemAddressRepository>();

  auto key_repo = std::make_shared<peer::InmemKeyRepository>();

  auto protocol_repo = std::make_shared<peer::InmemProtocolRepository>();

  auto peer_repo = std::make_unique<peer::PeerRepositoryImpl>(
      std::move(addr_repo), std::move(key_repo), std::move(protocol_repo));

  return std::make_shared<host::BasicHost>(
      idmgr, std::move(network), std::move(peer_repo));
}
