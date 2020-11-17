/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_KADEMLIA_CONTENTID
#define LIBP2P_KADEMLIA_KADEMLIA_CONTENTID

#include <gsl/span>
#include <vector>

#include <boost/optional.hpp>

namespace libp2p::protocol::kademlia {

  /// DHT key. Contains sha256 hash of Key bytes in CIDv0 format
  struct ContentId {
    std::vector<uint8_t> data;

    /// Ctor for consistency. Data will contain sha256 of empty input
    ContentId();

    explicit ContentId(const std::string &str);

    explicit ContentId(const std::vector<uint8_t> &v);

    // TODO(artem): use gsl::span<const T> + std::enable_if (integral types)
    ContentId(const void *bytes, size_t size);

    bool operator<(const ContentId &x) const {
      return data < x.data;
    }

    bool operator==(const ContentId &x) const {
      return data == x.data;
    }

    /// Validates and stores CID received from the network
    static boost::optional<ContentId> fromWire(const std::string &s);

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
