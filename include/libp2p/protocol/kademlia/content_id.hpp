/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_KADEMLIA_CONTENTID
#define LIBP2P_KADEMLIA_KADEMLIA_CONTENTID

#include <libp2p/common/byteutil.hpp>
#include <string_view>

namespace libp2p::protocol::kademlia {
  /// DHT key. Arbitrary bytes.
  using ContentId = common::ByteArray;

  ContentId makeKeySha256(std::string_view str);
}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KADEMLIA_KADEMLIA_CONTENTID
