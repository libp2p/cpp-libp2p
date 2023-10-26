/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/byteutil.hpp>
#include <string_view>

namespace libp2p::protocol::kademlia {
  /// DHT key. Arbitrary bytes.
  using ContentId = Bytes;

  ContentId makeKeySha256(std::string_view str);
}  // namespace libp2p::protocol::kademlia
