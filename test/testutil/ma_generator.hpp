/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TEST_TESTUTIL_LIBP2P_MA_GENERATOR_HPP
#define LIBP2P_TEST_TESTUTIL_LIBP2P_MA_GENERATOR_HPP

#include <iostream>

#include "libp2p/multi/multiaddress.hpp"
#include "testutil/outcome.hpp"

namespace testutil {
  class MultiaddressGenerator {
    using Multiaddress = libp2p::multi::Multiaddress;

   public:
    inline MultiaddressGenerator(std::string prefix, uint16_t start_port)
        : prefix_{std::move(prefix)}, current_port_{start_port} {}

    inline Multiaddress nextMultiaddress() {
      std::string ma = prefix_ + std::to_string(current_port_++);
      EXPECT_OUTCOME_TRUE(res, Multiaddress::create(ma));
      return res;
    }

   private:
    std::string prefix_;
    uint16_t current_port_;
  };
}  // namespace testutil

#endif  // LIBP2P_TEST_TESTUTIL_LIBP2P_MA_GENERATOR_HPP
