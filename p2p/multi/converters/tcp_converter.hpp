/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCPCONVERTER_HPP
#define KAGOME_TCPCONVERTER_HPP

#include <outcome/outcome.hpp>

namespace libp2p::multi::converters {

  /**
   * Converts a tcp part of a multiaddress (a port)
   * to bytes representation as a hex string
   */
  class TcpConverter {
   public:
    static auto addressToHex(std::string_view addr)
        -> outcome::result<std::string>;
  };

}  // namespace libp2p::multi::converters

#endif  // KAGOME_TCPCONVERTER_HPP
