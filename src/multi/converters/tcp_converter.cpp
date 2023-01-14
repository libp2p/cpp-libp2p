/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/tcp_converter.hpp>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>

namespace libp2p::multi::converters {

  outcome::result<std::string> TcpConverter::addressToHex(
      std::string_view addr) {
    for (auto c : addr) {
      if (std::isdigit(c) == 0) {
        return ConversionError::INVALID_ADDRESS;
      }
    }
    int64_t port_int = 0;
    try {
      port_int = std::stoi(std::string(addr));
    } catch (std::exception &e) {
      return ConversionError::INVALID_ADDRESS;
    }
    if (port_int >= 0 and port_int <= 65535) {
      return common::int_to_hex(port_int, 4);
    }
    return ConversionError::INVALID_ADDRESS;
  }

  outcome::result<common::ByteArray> TcpConverter::addressToBytes(
      std::string_view addr) {
    for (auto c : addr) {
      if (std::isdigit(c) == 0) {
        return ConversionError::INVALID_ADDRESS;
      }
    }
    int64_t port_int = 0;
    try {
      port_int = std::stoi(std::string(addr));
    } catch (std::exception &e) {
      return ConversionError::INVALID_ADDRESS;
    }
    if (port_int >= 0 and port_int <= 65535) {
      auto port_int16 = static_cast<uint16_t>(port_int);
      common::ByteArray bytes;
      common::putUint16BE(bytes, port_int16);
      return std::move(bytes);
    }
    return ConversionError::INVALID_ADDRESS;
  }

}  // namespace libp2p::multi::converters
