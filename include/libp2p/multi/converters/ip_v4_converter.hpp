/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/types.hpp>
#include <qtils/outcome.hpp>

namespace libp2p::multi::converters {

  /**
   * Converts an ip4 part of a multiaddress (an IP address)
   * to bytes representation as a hex string
   */
  class IPv4Converter {
   public:
    static outcome::result<Bytes> addressToBytes(std::string_view addr);
  };

}  // namespace libp2p::multi::converters
