/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/content_identifier.hpp>

namespace libp2p::multi {

  ContentIdentifier::ContentIdentifier(Version version,
                                       MulticodecType::Code content_type,
                                       Multihash content_address)
      : version{version},
        content_type{content_type},
        content_address{std::move(content_address)} {}

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
