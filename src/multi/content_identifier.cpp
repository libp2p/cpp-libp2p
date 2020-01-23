/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/content_identifier.hpp>

#include <boost/format.hpp>
#include <libp2p/common/hexutil.hpp>

namespace libp2p::multi {

  ContentIdentifier::ContentIdentifier(Version version,
                                       MulticodecType::Code content_type,
                                       Multihash content_address)
      : version{version},
        content_type{content_type},
        content_address{std::move(content_address)} {}

  std::string ContentIdentifier::toPrettyString(const std::string &base) const {
    /// TODO(Harrm) FIL-14: hash type is a subset of multicodec type, better
    /// move them to one place
    std::string hash_type = MulticodecType::getName(
        static_cast<MulticodecType::Code>(content_address.getType()));
    std::string hash_hex = common::hex_lower(content_address.getHash());
    std::string hash_length =
        std::to_string(content_address.getHash().size() * 8);
    std::string v = "cidv" + std::to_string(static_cast<uint64_t>(version));
    return (boost::format("%1% - %2% - %3% - %4%-%5%-%6%") % base % v
            % MulticodecType::getName(content_type) % hash_type % hash_length
            % hash_hex)
        .str();
  }

  bool ContentIdentifier::operator==(const ContentIdentifier &c) const {
    return version == c.version and content_type == c.content_type
        and content_address == c.content_address;
  }

  bool ContentIdentifier::operator<(const ContentIdentifier &c) const {
    return version < c.version
        || (version == c.version
            && (content_type < c.content_type
                || (content_type == c.content_type
                    && content_address < c.content_address)));
  }

}  // namespace libp2p::multi
