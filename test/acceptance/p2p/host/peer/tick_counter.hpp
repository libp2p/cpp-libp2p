/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TEST_ACCEPTANCE_LIBP2P_HOST_PEER_TICK_COUNTER_HPP
#define LIBP2P_TEST_ACCEPTANCE_LIBP2P_HOST_PEER_TICK_COUNTER_HPP

#include <atomic>

#include <gtest/gtest.h>

/**
 * @class TickCounter helper class which ensures that
 * number of interactions is correct
 */
struct TickCounter {
  TickCounter(size_t client_index, size_t server_index, uint32_t times)
      : ci{client_index},
        si{server_index},
        required_count{times},
        ticks_count{0u} {}

  void tick() {
    ++ticks_count;
  }

  void checkTicksCount() const {
    auto actual_count = ticks_count.load();
    ASSERT_EQ(actual_count, required_count)  // NOLINT
        << "sending messages from <" << ci << "> client to <" << si
        << "> server "
        << "required messages count <" << required_count
        << ">count doesn't match actual count <" << actual_count << ">";
  }

 private:
  const size_t ci;                  ///< client index
  const size_t si;                  ///< server index
  const size_t required_count;      ///< number of ticks to match
  std::atomic<size_t> ticks_count;  ///< current number of ticks
};

#endif  // LIBP2P_TEST_ACCEPTANCE_LIBP2P_HOST_PEER_TICK_COUNTER_HPP
