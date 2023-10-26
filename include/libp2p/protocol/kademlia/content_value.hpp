/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/byteutil.hpp>

namespace libp2p::protocol::kademlia {

  /// DHT value
  using ContentValue = std::vector<uint8_t>;

}  // namespace libp2p::protocol::kademlia
