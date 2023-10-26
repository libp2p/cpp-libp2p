/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/content_id.hpp>

#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/multi/content_identifier.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>

namespace libp2p::protocol::kademlia {
  ContentId makeKeySha256(std::string_view str) {
    auto digest_res = crypto::sha256(std::span(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t *>(str.data()),
        str.size()));
    BOOST_ASSERT(digest_res.has_value());

    auto mhash_res = libp2p::multi::Multihash::create(
        libp2p::multi::HashType::sha256, digest_res.value());
    BOOST_ASSERT(mhash_res.has_value());

    return multi::ContentIdentifierCodec::encodeCIDV1(
        libp2p::multi::MulticodecType::Code::RAW, mhash_res.value());
  }
}  // namespace libp2p::protocol::kademlia
