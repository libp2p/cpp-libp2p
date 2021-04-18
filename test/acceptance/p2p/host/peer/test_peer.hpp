/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_HOST_TEST_PEER_HPP
#define LIBP2P_HOST_TEST_PEER_HPP

#include <future>
#include <thread>

#include <libp2p/injector/host_injector.hpp>
#include <libp2p/network/cares/cares.hpp>
#include <libp2p/protocol/echo.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <testutil/async/impl/clock_impl.hpp>
#include <testutil/outcome.hpp>

struct TickCounter;

/**
 * @class Peer implements test version of a peer
 * to test libp2p basic functionality
 */
class Peer {
  template <class T>
  using sptr = std::shared_ptr<T>;

  using Multiaddress = libp2p::multi::Multiaddress;
  using PeerInfo = libp2p::peer::PeerInfo;
  using Stream = libp2p::connection::Stream;
  using BasicHost = libp2p::host::BasicHost;
  using KeyPair = libp2p::crypto::KeyPair;
  using MuxedConnectionConfig = libp2p::muxer::MuxedConnectionConfig;
  using Host = libp2p::Host;
  using Echo = libp2p::protocol::Echo;
  using BoostRandomGenerator = libp2p::crypto::random::BoostRandomGenerator;
  using Ed25519Provider = libp2p::crypto::ed25519::Ed25519Provider;
  using HmacProvider = libp2p::crypto::hmac::HmacProvider;
  using RsaProvider = libp2p::crypto::rsa::RsaProvider;
  using EcdsaProvider = libp2p::crypto::ecdsa::EcdsaProvider;
  using Secp256k1Provider = libp2p::crypto::secp256k1::Secp256k1Provider;
  using CryptoProvider = libp2p::crypto::CryptoProvider;

  using Context = boost::asio::io_context;
  using Scheduler = libp2p::basic::Scheduler;

 public:
  using Duration = libp2p::clock::SteadyClockImpl::Duration;

  /**
   * @brief constructs peer
   * @param timeout represents how long server and clients should work
   * @param secure - use SECIO when true, otherwise - Plaintext
   */
  explicit Peer(Duration timeout, bool secure);

  ~Peer() { wait(); }

  /**
   * @brief schedules server start
   * @param address address to listen
   * @param pp promise to set when peer info is obtained
   */
  void startServer(const Multiaddress &address,
                   std::shared_ptr<std::promise<PeerInfo>> promise);

  /**
   * @brief schedules start of client session
   * @param pinfo server peer info
   * @param message_count number of messages to send
   * @param tester object for testing purposes
   */
  void startClient(const PeerInfo &pinfo, size_t message_count,
                   sptr<TickCounter> tester);

  /**
   * @brief wait for all threads to join
   */
  void wait();

 private:
  sptr<BasicHost> makeHost(const KeyPair &keyPair);

  static libp2p::network::c_ares::Ares cares_;  ///< c-ares library instance
  MuxedConnectionConfig muxed_config_;          ///< muxed connection config
  const Duration timeout_;                      ///< operations timeout
  sptr<Context> context_;                       ///< io context
  std::thread thread_;                          ///< peer working thread
  sptr<Host> host_;                             ///< first host
  sptr<Echo> echo_;                             ///< echo protocol
  sptr<BoostRandomGenerator> random_provider_;  ///< random provider
  sptr<Ed25519Provider> ed25519_provider_;      ///< ed25519 provider
  sptr<RsaProvider> rsa_provider_;              ///< rsa provider
  sptr<EcdsaProvider> ecdsa_provider_;          ///< ecdsa provider
  sptr<Secp256k1Provider> secp256k1_provider_;  ///< secp256k1 provider
  sptr<HmacProvider> hmac_provider_;            ///< hmac provider
  sptr<CryptoProvider> crypto_provider_;        ///< crypto provider
  sptr<Scheduler> scheduler_;                   ///< scheduler
  const bool secure_;                           ///< use SECIO or not
};

#endif  // LIBP2P_HOST_TEST_PEER_HPP
