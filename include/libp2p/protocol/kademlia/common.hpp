/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_COMMON_HPP
#define LIBP2P_KADEMLIA_COMMON_HPP

#include <cstring>
#include <string>
#include <unordered_set>

#include <libp2p/multi/content_identifier.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>

namespace libp2p::protocol::kademlia {

  enum class Error {
    SUCCESS = 0,
    NO_PEERS = 1,
    MESSAGE_PARSE_ERROR = 2,
    MESSAGE_SERIALIZE_ERROR = 3,
    UNEXPECTED_MESSAGE_TYPE = 4,
    STREAM_RESET = 5,
    VALUE_NOT_FOUND = 6,
    CONTENT_VALIDATION_FAILED = 7,
    TIMEOUT = 8
  };

  using libp2p::common::Hash256;

  /// DHT key
  struct ContentAddress {
    std::vector<uint8_t> data;

    ContentAddress()
        : data(multi::ContentIdentifierCodec::encodeCIDV0("", 1)) {}

    explicit ContentAddress(const std::string &str)
        : data(multi::ContentIdentifierCodec::encodeCIDV0(str.data(),
                                                          str.size())) {}

    explicit ContentAddress(const std::vector<uint8_t> &v)
        : data(multi::ContentIdentifierCodec::encodeCIDV0(v.data(), v.size())) {
    }

    ContentAddress(const void *bytes, size_t size)
        : data(multi::ContentIdentifierCodec::encodeCIDV0(bytes, size)) {}

    bool operator<(const ContentAddress &x) const {
      return data < x.data;
    }

    bool operator==(const ContentAddress &x) const {
      return data == x.data;
    }

    static boost::optional<ContentAddress> fromWire(
        // TODO(artem): const vector needed
        std::vector<uint8_t> &v) {
      outcome::result<multi::ContentIdentifier> res =
          multi::ContentIdentifierCodec::decode(v);
      if (!res || res.value().content_address.getType() != multi::sha256) {
        return {};
      }
      return ContentAddress(FromWire{}, res.value().content_address.toBuffer());
    }

   private:
    struct FromWire {};
    ContentAddress(FromWire, std::vector<uint8_t> v) : data(std::move(v)) {}
  };

  /// DHT value
  using Value = std::vector<uint8_t>;

  /// Set of peer Ids
  using PeerIdSet = std::unordered_set<peer::PeerId>;

  /// Vector of peer Ids
  using PeerIdVec = std::vector<peer::PeerId>;

  /// Set of peer Infos
  using PeerInfoSet = std::unordered_set<peer::PeerInfo>;

}  // namespace libp2p::protocol::kademlia

namespace std {
  template <>
  struct hash<libp2p::protocol::kademlia::ContentAddress> {
    size_t operator()(
        const libp2p::protocol::kademlia::ContentAddress &x) const {
      size_t h = 0;
      size_t sz = x.data.size();
      // N.B. x.data() is a hash itself
      const size_t n = sizeof(size_t);
      if (sz >= n) {
        memcpy(&h, &x.data[sz - n], n);
      }
      return h;
    }
  };
}  // namespace std

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia, Error);

#endif  // LIBP2P_KADEMLIA_COMMON_HPP
