/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_UDP_CONVERTER_HPP
#define LIBP2P_UDP_CONVERTER_HPP

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi::converters {

  /**
   * Converts a udp part of a multiaddress (a port)
   * to bytes representation as a hex string
   */
  class UdpConverter {
   public:
    static auto addressToHex(std::string_view addr)
        -> outcome::result<std::string>;
  };
}  // namespace libp2p::multi::converters

#endif  // LIBP2P_UDP_CONVERTER_HPP
