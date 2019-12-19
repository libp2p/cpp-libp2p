/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/dns_converter.hpp>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/multi/uvarint.hpp>

namespace libp2p::multi::converters {

  auto DnsConverter::addressToHex(std::string_view addr)
      -> outcome::result<std::string> {
    std::vector<uint8_t> bytes(addr.size());
    std::copy(addr.begin(), addr.end(), bytes.begin());
    auto hex = common::hex_lower(bytes);
    return UVarint{bytes.size()}.toHex() + hex;
  }
}
