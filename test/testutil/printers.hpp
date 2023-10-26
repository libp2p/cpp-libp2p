/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ostream>
#include <vector>

#include <libp2p/common/hexutil.hpp>

namespace std {

  void PrintTo(const std::vector<uint8_t> &v, std::ostream *os) {
    *os << libp2p::common::hex_upper(v);
  }

}  // namespace std
