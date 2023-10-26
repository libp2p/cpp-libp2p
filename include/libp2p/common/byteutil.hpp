/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include <libp2p/common/types.hpp>

namespace libp2p::common {

  /**
   * Put an 8-bit number into the byte array
   */
  Bytes &putUint8(Bytes &bytes, uint8_t n);

  /**
   * Put a 16-bit number into the byte array in Big Endian encoding
   */
  Bytes &putUint16BE(Bytes &bytes, uint16_t n);

  /**
   * Put a 16-bit number into the byte array in Little Endian encoding
   */
  Bytes &putUint16LE(Bytes &bytes, uint16_t n);

  /**
   * Put an 32-bit number into the byte array in Big Endian encoding
   */
  Bytes &putUint32BE(Bytes &bytes, uint32_t n);

  /**
   * Put a 32-bit number into the byte array in Little Endian encoding
   */
  Bytes &putUint32LE(Bytes &bytes, uint32_t n);

  /**
   * Put an 64-bit number into the byte array in Big Endian encoding
   */
  Bytes &putUint64BE(Bytes &bytes, uint64_t n);

  /**
   * Put a 64-bit number into the byte array in Little Endian encoding
   */
  Bytes &putUint64LE(Bytes &bytes, uint64_t n);

  /**
   * Convert value, to which the pointer (\param v) references, to the value of
   * (\tparam T)
   *
   * @example
   * std::vector<uint8_t> bytes_of_uint16{0x2A, 0x08};
   * uint16_t converted_value = convert<uint16_t>(bytes_of_uint16.data());
   * printf(converted_value);   // 10760 (2A08 in hex)
   */
  template <typename T>
  T convert(const void *v) {
    T t;
    memcpy(&t, v, sizeof(T));
    return t;
  }
}  // namespace libp2p::common

namespace std {
  template <>
  struct hash<libp2p::Bytes> {
    size_t operator()(const libp2p::Bytes &x) const;
  };
}  // namespace std
