/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/content_id.hpp>

#include <cstring>

#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/multi/content_identifier.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>

namespace libp2p::protocol::kademlia {

  ContentId::ContentId()
      : data(multi::ContentIdentifierCodec::encodeCIDV0("", 0)) {}

  ContentId::ContentId(std::string_view str) {
    auto digest_res = crypto::sha256(gsl::make_span(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t *>(str.data()),
        static_cast<ptrdiff_t>(str.size())));
    BOOST_ASSERT(digest_res.has_value());

    auto mhash_res = libp2p::multi::Multihash::create(
        libp2p::multi::HashType::sha256, digest_res.value());
    BOOST_ASSERT(mhash_res.has_value());

    data = multi::ContentIdentifierCodec::encodeCIDV1(
        libp2p::multi::MulticodecType::Code::RAW, mhash_res.value());
  }

  ContentId::ContentId(const std::vector<uint8_t> &v)
      : data(multi::ContentIdentifierCodec::encodeCIDV0(v.data(), v.size())) {}

  boost::optional<ContentId> ContentId::fromWire(std::string_view str) {
    gsl::span<const uint8_t> bytes(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t *>(str.data()),
        static_cast<ptrdiff_t>(str.size()));
    return fromWire(bytes);
  }

  boost::optional<ContentId> ContentId::fromWire(
      gsl::span<const uint8_t> bytes) {
    outcome::result<multi::ContentIdentifier> cid_res =
        multi::ContentIdentifierCodec::decode(bytes);
    if (cid_res.has_value()) {
      auto &cid = cid_res.value();
      if (cid.content_address.getType() == multi::sha256) {
        return ContentId(FromWire{}, bytes);
      }
    }
    return {};
  }

  ContentId::ContentId(FromWire, gsl::span<const uint8_t> bytes)
      : data(bytes.begin(), bytes.end()) {}

}  // namespace libp2p::protocol::kademlia

namespace std {

  std::size_t hash<libp2p::protocol::kademlia::ContentId>::operator()(
      const libp2p::protocol::kademlia::ContentId &x) const {
    size_t h = 0;
    size_t sz = x.data.size();
    // N.B. x.data() is a hash itself
    const size_t n = sizeof(size_t);
    if (sz >= n) {
      memcpy(&h, &x.data[sz - n], n);
    }
    return h;
  }

}  // namespace std
