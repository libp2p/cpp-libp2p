/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/dns_converter.hpp>

#include <libp2p/multi/uvarint.hpp>

namespace libp2p::multi::converters {
  outcome::result<Bytes> DnsConverter::addressToBytes(std::string_view addr) {
    outcome::result<Bytes> res = outcome::success();  // NRVO
    auto &bytes = res.value();
    bytes = UVarint(addr.size()).toVector();
    bytes.reserve(bytes.size() + addr.size());
    bytes.insert(bytes.end(), addr.begin(), addr.end());
    return res;
  }

}  // namespace libp2p::multi::converters
