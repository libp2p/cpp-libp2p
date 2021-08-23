/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "acceptance/p2p/host/peer/test_peer.hpp"

#include <gtest/gtest.h>
#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/crypto/ecdsa_provider/ecdsa_provider_impl.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/hmac_provider/hmac_provider_impl.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>
#include <libp2p/crypto/key_validator/key_validator_impl.hpp>
#include <libp2p/crypto/rsa_provider/rsa_provider_impl.hpp>
#include <libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp>
#include <libp2p/network/impl/dnsaddr_resolver_impl.hpp>
#include <libp2p/security/plaintext/exchange_message_marshaller_impl.hpp>
#include <libp2p/security/secio/exchange_message_marshaller_impl.hpp>
#include <libp2p/security/secio/propose_message_marshaller_impl.hpp>
#include "acceptance/p2p/host/peer/tick_counter.hpp"
#include "acceptance/p2p/host/protocol/client_test_session.hpp"

using namespace libp2p;  // NOLINT

libp2p::network::c_ares::Ares Peer::cares_;

Peer::Peer(Peer::Duration timeout, bool secure)
    : muxed_config_{1024576, 1000},
      timeout_{timeout},
      context_{std::make_shared<Context>()},
      echo_{std::make_shared<Echo>()},
      random_provider_{std::make_shared<BoostRandomGenerator>()},
      ed25519_provider_{
          std::make_shared<crypto::ed25519::Ed25519ProviderImpl>()},
      rsa_provider_{std::make_shared<crypto::rsa::RsaProviderImpl>()},
      ecdsa_provider_{std::make_shared<crypto::ecdsa::EcdsaProviderImpl>()},
      secp256k1_provider_{
          std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>()},
      hmac_provider_{std::make_shared<crypto::hmac::HmacProviderImpl>()},
      crypto_provider_{std::make_shared<crypto::CryptoProviderImpl>(
          random_provider_, ed25519_provider_, rsa_provider_, ecdsa_provider_,
          secp256k1_provider_, hmac_provider_)},
      scheduler_{std::make_shared<basic::SchedulerImpl>(
          std::make_shared<basic::AsioSchedulerBackend>(context_),
          basic::Scheduler::Config{})},
      secure_{secure} {
  EXPECT_OUTCOME_TRUE_MSG(
      keys, crypto_provider_->generateKeys(crypto::Key::Type::Ed25519),
      "failed to generate keys");
  host_ = makeHost(keys);

  auto handler = [this](std::shared_ptr<Stream> result) {
    echo_->handle(result);
  };
  host_->setProtocolHandler(echo_->getProtocolId(), handler);
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

void Peer::startClient(const peer::PeerInfo &pinfo, size_t message_count,
                       Peer::sptr<TickCounter> counter) {
  context_->post([this, server_id = pinfo.id.toBase58(), pinfo, message_count,
                  counter = std::move(counter)]() mutable {
    this->host_->newStream(
        pinfo, echo_->getProtocolId(),
        [server_id = std::move(server_id), ping_times = message_count,
         counter = std::move(counter)](
            outcome::result<sptr<Stream>> rstream) mutable {
          // get stream
          EXPECT_OUTCOME_TRUE_MSG(stream, rstream,
                                  "failed to connect to server: " + server_id);
          // make client session
          auto client =
              std::make_shared<protocol::ClientTestSession>(stream, ping_times);
          // handle session
          client->handle(
              [server_id = std::move(server_id), client,
               counter = std::move(counter)](
                  outcome::result<std::vector<uint8_t>> res) mutable {
                // count message exchange
                counter->tick();
                // ensure message returned
                EXPECT_OUTCOME_TRUE_MSG(
                    vec, res,
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
  host_->stop();
}

Peer::sptr<host::BasicHost> Peer::makeHost(const crypto::KeyPair &keyPair) {
  auto crypto_provider = std::make_shared<crypto::CryptoProviderImpl>(
      random_provider_, ed25519_provider_, rsa_provider_, ecdsa_provider_,
      secp256k1_provider_, hmac_provider_);

  auto key_validator =
      std::make_shared<crypto::validator::KeyValidatorImpl>(crypto_provider);

  auto key_marshaller = std::make_shared<crypto::marshaller::KeyMarshallerImpl>(
      std::move(key_validator));

  auto idmgr =
      std::make_shared<peer::IdentityManagerImpl>(keyPair, key_marshaller);

  auto multiselect =
      std::make_shared<protocol_muxer::multiselect::Multiselect>();

  auto router = std::make_shared<network::RouterImpl>();

  auto exchange_msg_marshaller =
      std::make_shared<security::plaintext::ExchangeMessageMarshallerImpl>(
          key_marshaller);

  std::vector<std::shared_ptr<security::SecurityAdaptor>> security_adaptors;

  if (secure_) {
    security_adaptors.emplace_back(std::make_shared<security::Secio>(
        std::make_shared<crypto::random::BoostRandomGenerator>(),
        crypto_provider,
        std::make_shared<security::secio::ProposeMessageMarshallerImpl>(),
        std::make_shared<security::secio::ExchangeMessageMarshallerImpl>(),
        idmgr, key_marshaller, hmac_provider_));
  } else {
    security_adaptors.emplace_back(std::make_shared<security::Plaintext>(
        std::move(exchange_msg_marshaller), idmgr, std::move(key_marshaller)));
  }

  std::vector<std::shared_ptr<muxer::MuxerAdaptor>> muxer_adaptors = {
      std::make_shared<muxer::Yamux>(muxed_config_, scheduler_, nullptr)};

  auto upgrader = std::make_shared<transport::UpgraderImpl>(
      multiselect, std::move(security_adaptors), std::move(muxer_adaptors));

  std::vector<std::shared_ptr<transport::TransportAdaptor>> transports = {
      std::make_shared<transport::TcpTransport>(context_, std::move(upgrader))};

  auto tmgr =
      std::make_shared<network::TransportManagerImpl>(std::move(transports));

  auto bus = std::make_shared<libp2p::event::Bus>();

  auto cmgr = std::make_shared<network::ConnectionManagerImpl>(bus);

  auto listener = std::make_shared<network::ListenerManagerImpl>(
      multiselect, std::move(router), tmgr, cmgr);

  auto dialer = std::make_unique<network::DialerImpl>(multiselect, tmgr, cmgr,
                                                      listener, scheduler_);

  auto network = std::make_unique<network::NetworkImpl>(
      std::move(listener), std::move(dialer), cmgr);

  auto dnsaddr_resolver =
      std::make_shared<network::DnsaddrResolverImpl>(context_, cares_);

  auto addr_repo =
      std::make_shared<peer::InmemAddressRepository>(dnsaddr_resolver);

  auto key_repo = std::make_shared<peer::InmemKeyRepository>();

  auto protocol_repo = std::make_shared<peer::InmemProtocolRepository>();

  auto peer_repo = std::make_unique<peer::PeerRepositoryImpl>(
      std::move(addr_repo), std::move(key_repo), std::move(protocol_repo));

  return std::make_shared<host::BasicHost>(idmgr, std::move(network),
                                           std::move(peer_repo), std::move(bus),
                                           std::move(tmgr));
}
