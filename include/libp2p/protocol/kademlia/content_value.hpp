/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_KADEMLIA_CONTENTVALUE
#define LIBP2P_KADEMLIA_KADEMLIA_CONTENTVALUE

#include <libp2p/common/byteutil.hpp>

namespace libp2p::protocol::kademlia {

  /// DHT value
  using ContentValue = std::vector<uint8_t>;

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KADEMLIA_KADEMLIA_CONTENTVALUE
