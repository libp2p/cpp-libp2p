/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "multi/converters/ip_v6_converter.hpp"

#include <string>

#include <boost/asio/ip/address_v6.hpp>
#include <outcome/outcome.hpp>
#include "common/hexutil.hpp"
#include "multi/converters/conversion_error.hpp"
#include "multi/multiaddress_protocol_list.hpp"

namespace libp2p::multi::converters {

  auto IPv6Converter::addressToHex(std::string_view addr)
      -> outcome::result<std::string> {
    boost::system::error_code ec;
    auto address = boost::asio::ip::make_address_v6(addr, ec);
    if (ec) {
      return ec;
    }
    auto iip = address.to_bytes();
    auto hex = common::hex_lower(iip);
    return hex;
  }

}  // namespace libp2p::multi::converters
