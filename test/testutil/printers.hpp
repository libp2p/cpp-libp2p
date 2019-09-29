/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TEST_TESTUTIL_PRINTERS_HPP
#define LIBP2P_TEST_TESTUTIL_PRINTERS_HPP

#include <vector>
#include <ostream>

#include <libp2p/common/hexutil.hpp>

namespace std {

  void PrintTo(const std::vector<uint8_t> &v, std::ostream* os) {
    *os << libp2p::common::hex_upper(v);
  }

}

#endif  // LIBP2P_TEST_TESTUTIL_PRINTERS_HPP
