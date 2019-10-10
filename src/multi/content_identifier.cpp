/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/content_identifier.hpp>

#include <libp2p/common/hexutil.hpp>

namespace libp2p::multi {

  ContentIdentifier::ContentIdentifier(Version version,
                                       MulticodecType::Code content_type,
                                       Multihash content_address)
      : version{version},
        content_type{content_type},
        content_address{std::move(content_address)} {}

  std::string ContentIdentifier::toPrettyString(std::string_view base) {
    /// TODO(Harrm): hash type is a subset of multicodec type, better move them
    /// to one place
    std::string hash_type = MulticodecType::getName(
        static_cast<MulticodecType::Code>(content_address.getType()));
    std::string hash_hex = common::hex_lower(content_address.getHash());
    return MulticodecType::getName(content_type) + " - " + hash_type + " - " + hash_hex;
  }

  bool ContentIdentifier::operator==(const ContentIdentifier &c) const {
    return version == c.version and content_type == c.content_type
        and content_address == c.content_address;
  }

  bool ContentIdentifier::operator!=(const ContentIdentifier &c) const {
    return !(*this == c);
  }

}  // namespace libp2p::multi
