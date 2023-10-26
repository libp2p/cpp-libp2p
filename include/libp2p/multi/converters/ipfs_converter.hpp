/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/types.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi::converters {

  /**
   * Converts an ipfs part of a multiaddress (encoded in base58)
   * to bytes representation as a hex string
   */
  class IpfsConverter {
   public:
    [[deprecated("Use `common::hex_lower(addressToBytes(...))` instead")]]  //
    static outcome::result<std::string>
    addressToHex(std::string_view addr);

    static outcome::result<Bytes> addressToBytes(std::string_view addr);
  };

}  // namespace libp2p::multi::converters
