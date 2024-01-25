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
   * Converts a DNS part of a multiaddress (a hostname)
   * to bytes representation as a hex string
   */
  class DnsConverter {
   public:
    static outcome::result<Bytes> addressToBytes(std::string_view addr);
  };

}  // namespace libp2p::multi::converters
