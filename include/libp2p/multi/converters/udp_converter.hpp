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
   * Converts a udp part of a multiaddress (a port)
   * to bytes representation as a hex string
   */
  class UdpConverter {
   public:
    static outcome::result<Bytes> addressToBytes(std::string_view addr);
  };
}  // namespace libp2p::multi::converters
