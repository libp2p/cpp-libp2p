/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_ACCEPTANCE_LIBP2P_HOST_TEST_PEER_HPP
#define KAGOME_TEST_ACCEPTANCE_LIBP2P_HOST_TEST_PEER_HPP

#include <future>
#include <thread>

#include "clock/impl/clock_impl.hpp"
#include "libp2p/injector/host_injector.hpp"
#include "libp2p/protocol/echo.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;

struct TickCounter;

/**
 * @class Peer implements test version of a peer
 * to test libp2p basic functionality
 */
class Peer {
  template <class T>
  using sptr = std::shared_ptr<T>;

  using Context = boost::asio::io_context;
  using Stream = connection::Stream;

 public:
  using Duration = kagome::clock::SteadyClockImpl::Duration;

  /**
   * @brief constructs peer
   * @param timeout represents how long server and clients should work
   */
  explicit Peer(Duration timeout);

  /**
   * @brief schedules server start
   * @param address address to listen
   * @param pp promise to set when peer info is obtained
   */
  void startServer(const multi::Multiaddress &address,
                   std::shared_ptr<std::promise<peer::PeerInfo>> promise);

  /**
   * @brief schedules start of client session
   * @param pinfo server peer info
   * @param message_count number of messages to send
   * @param tester object for testing purposes
   */
  void startClient(const peer::PeerInfo &pinfo,
                   size_t message_count,
                   sptr<TickCounter> tester);

  /**
   * @brief wait for all threads to join
   */
  void wait();

 private:
  sptr<host::BasicHost> makeHost(crypto::KeyPair keyPair);

  muxer::MuxedConnectionConfig muxed_config_;  ///< muxed connection config
  const Duration timeout_;                     ///< operations timeout
  sptr<Context> context_;                      ///< io context
  std::thread thread_;                         ///< peer working thread
  sptr<Host> host_;                            ///< host
  sptr<protocol::Echo> echo_;                  ///< echo protocol
  sptr<crypto::random::BoostRandomGenerator>
      random_provider_;                       ///< random provider
  sptr<crypto::KeyGenerator> key_generator_;  ///< key generator
};

#endif  // KAGOME_TEST_ACCEPTANCE_LIBP2P_HOST_TEST_PEER_HPP
