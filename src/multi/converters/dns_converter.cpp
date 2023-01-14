/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/dns_converter.hpp>

#include <boost/algorithm/hex.hpp>

#include <libp2p/multi/uvarint.hpp>

namespace libp2p::multi::converters {

  outcome::result<std::string> DnsConverter::addressToHex(
      std::string_view addr) {
    outcome::result<std::string> res = outcome::success();  // NRVO
    auto &hex = res.value();
    hex = UVarint(addr.size()).toHex();
    hex.reserve(hex.size() + addr.size() * 2);
    boost::algorithm::hex_lower(addr.begin(), addr.end(),
                                std::back_inserter(hex));
    return res;
  }

  outcome::result<common::ByteArray> DnsConverter::addressToBytes(
      std::string_view addr) {
    outcome::result<common::ByteArray> res = outcome::success();  // NRVO
    auto &bytes = res.value();
    bytes = UVarint(addr.size()).toVector();
    bytes.reserve(bytes.size() + addr.size());
    bytes.insert(bytes.end(), addr.begin(), addr.end());
    return res;
  }

}  // namespace libp2p::multi::converters
