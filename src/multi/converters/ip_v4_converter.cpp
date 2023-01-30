/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/ip_v4_converter.hpp>

#include <boost/asio/ip/address_v4.hpp>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>

namespace libp2p::multi::converters {

  outcome::result<std::string> IPv4Converter::addressToHex(
      std::string_view addr) {
    boost::system::error_code ec;
    auto address = boost::asio::ip::make_address_v4(addr, ec);
    if (ec) {
      return ConversionError::INVALID_ADDRESS;
    }
    uint32_t iip = address.to_uint();
    auto hex = common::int_to_hex(iip);
    hex = std::string(8 - hex.length(), '0') + hex;
    return hex;
  }

  outcome::result<common::ByteArray> IPv4Converter::addressToBytes(
      std::string_view addr) {
    boost::system::error_code ec;
    auto address = boost::asio::ip::make_address_v4(addr, ec);
    if (ec) {
      return ConversionError::INVALID_ADDRESS;
    }
    auto ip_bytes = address.to_bytes();
    return common::ByteArray(ip_bytes.begin(), ip_bytes.end());
  }

}  // namespace libp2p::multi::converters
