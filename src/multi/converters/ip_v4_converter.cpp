/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/ip_v4_converter.hpp>

#include <boost/asio/ip/address_v4.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>

namespace libp2p::multi::converters {
  outcome::result<Bytes> IPv4Converter::addressToBytes(std::string_view addr) {
    boost::system::error_code ec;
    auto address = boost::asio::ip::make_address_v4(addr, ec);
    if (ec) {
      return Q_ERROR(ConversionError::INVALID_ADDRESS);
    }
    auto ip_bytes = address.to_bytes();
    return Bytes(ip_bytes.begin(), ip_bytes.end());
  }

}  // namespace libp2p::multi::converters
