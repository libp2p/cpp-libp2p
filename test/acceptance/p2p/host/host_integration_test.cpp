/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host/basic_host/basic_host.hpp"

#include <chrono>
#include <future>

#include <gtest/gtest.h>
#include "acceptance/p2p/host/peer/test_peer.hpp"
#include "acceptance/p2p/host/peer/tick_counter.hpp"
#include "testutil/ma_generator.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace libp2p;

using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;
using Duration = libp2p::clock::SteadyClockImpl::Duration;

/**
 * @brief host integration test configuration
 */
struct HostIntegrationTestConfig {
  size_t peer_count;           ///< how many peers to create
  size_t ping_times;           ///< how many messages to send
  uint16_t start_port;         ///< start port number for server addresses
  Duration operation_timeout;  ///< how long to run io_context
  Duration future_timeout;     ///< how long to wait to obtain peer info
  Duration system_timeout;  ///< how long to wait before starting clients after
                            ///< all peer info obtained
  bool secured;             ///< use SECIO if true, otherwise Plaintext
};

/*
 * @brief host integration test fixture
 */
struct HostIntegrationTest
    : public ::testing::TestWithParam<HostIntegrationTestConfig> {
  template <class T>
  using sptr = std::shared_ptr<T>;

  using PeerPromise = std::promise<peer::PeerInfo>;
  using PeerFuture = std::shared_future<peer::PeerInfo>;

  void TearDown() override {
    peers.clear();
    peerinfo_futures.clear();
  }

  std::vector<sptr<Peer>> peers;
  std::vector<PeerFuture> peerinfo_futures;
};

/**
 * @given a predefined number of peers each represents an echo server
 * @when each peer starts its server, obtains `peer info`
 * @and sets value to `peer info` promises
 * @and initiates client sessions to all other servers
 * @then all clients interact with all servers predefined number of times
 */
TEST_P(HostIntegrationTest, InteractAllToAllSuccess) {

  testutil::prepareLoggers();

  const auto [peer_count, ping_times, start_port, timeout, future_timeout,
              system_timeout, secured] = GetParam();
  const auto addr_prefix = "/ip4/127.0.0.1/tcp/";
  testutil::MultiaddressGenerator ma_generator(addr_prefix, start_port);

  // create peers
  peers.reserve(peer_count);
  peerinfo_futures.reserve(peer_count);

  // initialize `peer info` promises and futures and addresses
  for (size_t i = 0; i < peer_count; ++i) {
    auto promise = std::make_shared<PeerPromise>();
    peerinfo_futures.push_back(promise->get_future());

    auto peer = std::make_shared<Peer>(timeout, secured);
    auto ma = ma_generator.nextMultiaddress();
    peer->startServer(ma, std::move(promise));
    peers.push_back(std::move(peer));
  }

  // need to wait for `peer info` values before starting client sessions
  for (auto &f : peerinfo_futures) {
    auto status = f.wait_for(future_timeout);
    ASSERT_EQ(status, std::future_status::ready);
  }

  // wait for server sockets to start
  std::this_thread::sleep_for(system_timeout);

  // reserve vector of message counters
  std::vector<std::shared_ptr<TickCounter>> counters;
  counters.reserve(peer_count * peer_count);

  // start client sessions from all peers to all other peers
  for (size_t i = 0; i < peer_count; ++i) {
    for (size_t j = 0; j < peer_count; ++j) {
      if (secured and i == j) {
        // SECIO does not allow communicating itself
        continue;
      }
      const auto &pinfo = peerinfo_futures[j].get();
      auto counter = std::make_shared<TickCounter>(i, j, ping_times);
      peers[i]->startClient(pinfo, ping_times, counter);
    }
  }

  // wait for peers to finish their jobs
  for (const auto &p : peers) {
    p->wait();
  }

  // check that all messages have been sent
  for (auto &counter : counters) {
    counter->checkTicksCount();
  }
}

namespace {
  using Config = HostIntegrationTestConfig;
}

INSTANTIATE_TEST_CASE_P(AllTestCases, HostIntegrationTest,
                        ::testing::Values(
                            // ports are not freed, so new ports each time
                            Config{1u, 1u, 40510u, 2s, 2s, 200ms, false},
                            Config{2u, 1u, 40510u, 2s, 2s, 200ms, false},
                            Config{2u, 1u, 40510u, 5s, 2s, 200ms, true}
                            /*, TODO(igor-egorov) FIL-143 enable test for more than two SECIO peers
                            Config{3u, 1u, 40510u, 5s, 2s, 200ms, true}*/));
