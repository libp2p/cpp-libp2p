/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_IPV4CONVERTER_HPP
#define LIBP2P_IPV4CONVERTER_HPP

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi::converters {

  /**
   * Converts an ip4 part of a multiaddress (an IP address)
   * to bytes representation as a hex string
   */
  class IPv4Converter {
   public:
    static auto addressToHex(std::string_view addr)
        -> outcome::result<std::string>;
  };

}  // namespace libp2p::multi::converters

#endif  // LIBP2P_IPV4CONVERTER_HPP
