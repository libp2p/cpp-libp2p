/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_CONTENT_ADDRESS_HPP
#define LIBP2P_KADEMLIA_CONTENT_ADDRESS_HPP

#include <vector>
#include <gsl/span>

#include <boost/optional.hpp>

namespace libp2p::protocol::kademlia {

  /// DHT key. Contains sha256 hash of Key bytes in CIDv0 format
  struct ContentAddress {
    std::vector<uint8_t> data;

    /// Ctor for consistency. Data will contain sha256 of empty input
    ContentAddress();

    explicit ContentAddress(const std::string &str);

    explicit ContentAddress(const std::vector<uint8_t> &v);

    // TODO(artem): use gsl::span<const T> + std::enable_if (integral types)
    ContentAddress(const void *bytes, size_t size);

    bool operator<(const ContentAddress &x) const {
      return data < x.data;
    }

    bool operator==(const ContentAddress &x) const {
      return data == x.data;
    }

    /// Validates and stores CID received from the network
    static boost::optional<ContentAddress> fromWire(
        const std::string &s);

    static boost::optional<ContentAddress> fromWire(
        gsl::span<const uint8_t> bytes);

   private:
    struct FromWire {};
    ContentAddress(FromWire, std::vector<uint8_t> v);
  };

} //namespace libp2p::protocol::kademlia

namespace std {
  template <>
  struct hash<libp2p::protocol::kademlia::ContentAddress> {
    std::size_t operator()(
        const libp2p::protocol::kademlia::ContentAddress &x) const;
  };
}  // namespace std

#endif  // LIBP2P_KADEMLIA_CONTENT_ADDRESS_HPP
