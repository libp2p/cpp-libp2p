/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_DNS_CONVERTER_HPP
#define LIBP2P_DNS_CONVERTER_HPP

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi::converters {

  /**
   * Converts a DNS part of a multiaddress (a hostname)
   * to bytes representation as a hex string
   */
  class DnsConverter {
   public:
    static auto addressToHex(std::string_view addr)
    -> outcome::result<std::string>;
  };

}  // namespace libp2p::multi::converters

#endif  // LIBP2P_DNS_CONVERTER_HPP
