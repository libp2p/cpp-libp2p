/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/tcp_converter.hpp>

#include <string>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi::converters {

  auto TcpConverter::addressToHex(std::string_view addr)
      -> outcome::result<std::string> {
    int64_t n = 0;
    for (auto &c : addr) {
      if (std::isdigit(c) == 0) {
        return ConversionError::INVALID_ADDRESS;
      }
    }
    try {
      n = std::stoi(std::string(addr));
    } catch (std::exception &e) {
      return ConversionError::INVALID_ADDRESS;
    }
    if (n == 0) {
      return "0000";
    }
    if (n < 65536 && n > 0) {
      return common::int_to_hex(n, 4);
    }
    return ConversionError::INVALID_ADDRESS;
  }
}  // namespace libp2p::multi::converters
