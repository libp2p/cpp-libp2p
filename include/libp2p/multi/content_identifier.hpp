/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONTENT_IDENTIFIER_HPP
#define LIBP2P_CONTENT_IDENTIFIER_HPP

#include <vector>

#include <libp2p/multi/multihash.hpp>
#include <libp2p/multi/multicodec_type.hpp>

namespace libp2p::multi {

  /**
   *
   *
   * @note multibase may be omitted in non text-based protocols and is generally
   * needed only for CIDs serialized to a string, so it is not present in this
   * structure
   */
  class ContentIdentifier {
   public:
    enum class Version { V0 = 0, V1 = 1};

    ContentIdentifier(Version version,
                      MulticodecType::Code content_type,
                      Multihash content_address);

    std::string toPrettyString(std::string_view base);

    bool operator==(const ContentIdentifier &c) const;
    bool operator!=(const ContentIdentifier &c) const;

    Version version;
    MulticodecType::Code content_type;
    Multihash content_address;
  };

}  // namespace libp2p::multi

#endif  // LIBP2P_CONTENT_IDENTIFIER_HPP
