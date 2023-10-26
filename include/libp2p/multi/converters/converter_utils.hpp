/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

#include <libp2p/common/types.hpp>
#include <libp2p/multi/multiaddress_protocol_list.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <span>

namespace libp2p::multi::converters {

  /**
   * Converts the given address string of the specified protocol
   * to a byte sequence that represents the address, if both
   * address and protocol were valid
   */
  outcome::result<Bytes> addressToBytes(const Protocol &protocol,
                                        std::string_view addr);

  /**
   * Converts the given address string of the specified protocol
   * to a hex string that represents the address, if both
   * address and protocol were valid
   */
  [[deprecated("Use `common::hex_lower(addressToBytes(...))` instead")]]  //
  outcome::result<std::string>
  addressToHex(const Protocol &protocol, std::string_view addr);

  /**
   * Converts the given multiaddr string to a byte sequence representing
   * the multiaddr, if provided multiaddr was valid
   */
  auto multiaddrToBytes(std::string_view multiaddr_str)
      -> outcome::result<Bytes>;

  /**
   * Converts the given byte sequence representing
   * a multiaddr to a string containing the multiaddr in a human-readable
   * format, if the provided byte sequence was a valid multiaddr
   */
  auto bytesToMultiaddrString(const Bytes &bytes)
      -> outcome::result<std::string>;

  /**
   * Converts the given byte sequence representing
   * a multiaddr to a string containing the multiaddr in a human-readable
   * format, if the provided byte sequence was a valid multiaddr
   */
  auto bytesToMultiaddrString(BytesIn bytes) -> outcome::result<std::string>;

}  // namespace libp2p::multi::converters
