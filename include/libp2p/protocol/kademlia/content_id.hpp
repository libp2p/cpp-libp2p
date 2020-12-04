/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_KADEMLIA_CONTENTID
#define LIBP2P_KADEMLIA_KADEMLIA_CONTENTID

#include <boost/optional.hpp>
#include <gsl/span>
#include <string_view>
#include <vector>

namespace libp2p::protocol::kademlia {

  /// DHT key. Contains sha256 hash of Key bytes in CIDv0 format
  struct ContentId {
    std::vector<uint8_t> data;

    /// Ctor for consistency. Data will contain sha256 of empty input
    ContentId();

    explicit ContentId(std::string_view str);

    explicit ContentId(const std::vector<uint8_t> &v);

    bool operator<(const ContentId &x) const {
      return data < x.data;
    }

    bool operator==(const ContentId &x) const {
      return data == x.data;
    }

    /// Validates and stores CID received from the network
    static boost::optional<ContentId> fromWire(std::string_view str);

    static boost::optional<ContentId> fromWire(gsl::span<const uint8_t> bytes);

   private:
    struct FromWire {};
    ContentId(FromWire, gsl::span<const uint8_t> bytes);
  };

}  // namespace libp2p::protocol::kademlia

namespace std {
  template <>
  struct hash<libp2p::protocol::kademlia::ContentId> {
    std::size_t operator()(
        const libp2p::protocol::kademlia::ContentId &x) const;
  };
}  // namespace std

#endif  // LIBP2P_KADEMLIA_KADEMLIA_CONTENTID
