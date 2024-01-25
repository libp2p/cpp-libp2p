/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/bytes.hpp>

namespace libp2p::common {
  /// Hash160 as a sequence of 20 bytes
  using Hash160 = qtils::BytesN<20>;
  /// Hash256 as a sequence of 32 bytes
  using Hash256 = qtils::BytesN<32>;
  /// Hash512 as a sequence of 64 bytes
  using Hash512 = qtils::BytesN<64>;
}  // namespace libp2p::common

namespace libp2p {
  using qtils::Bytes;
  using qtils::BytesIn;
  using qtils::BytesOut;
}  // namespace libp2p
