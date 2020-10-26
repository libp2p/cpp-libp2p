/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_KADEMLIA_CONTENTVALUE
#define LIBP2P_KADEMLIA_KADEMLIA_CONTENTVALUE

#include <gsl/span>
#include <vector>

#include <boost/optional.hpp>

namespace libp2p::protocol::kademlia {

  /// DHT value
  struct ContentValue : std::vector<uint8_t>{};

}  // namespace libp2p::protocol::kademlia

namespace std {
  template <>
  struct hash<libp2p::protocol::kademlia::ContentValue> {
    std::size_t operator()(
        const libp2p::protocol::kademlia::ContentValue &x) const;
  };
}  // namespace std

#endif  // LIBP2P_KADEMLIA_KADEMLIA_CONTENTVALUE
