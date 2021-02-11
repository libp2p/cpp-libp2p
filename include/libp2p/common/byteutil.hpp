/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BYTEUTIL_HPP
#define LIBP2P_BYTEUTIL_HPP

#include <cstdint>
#include <cstring>
#include <vector>

namespace libp2p::common {
  using ByteArray = std::vector<uint8_t>;

  /**
   * Put an 8-bit number into the byte array
   */
  ByteArray &putUint8(ByteArray &bytes, uint8_t n);

  /**
   * Put a 16-bit number into the byte array in Big Endian encoding
   */
  ByteArray &putUint16BE(ByteArray &bytes, uint16_t n);

  /**
   * Put a 16-bit number into the byte array in Little Endian encoding
   */
  ByteArray &putUint16LE(ByteArray &bytes, uint16_t n);

  /**
   * Put an 32-bit number into the byte array in Big Endian encoding
   */
  ByteArray &putUint32BE(ByteArray &bytes, uint32_t n);

  /**
   * Put a 32-bit number into the byte array in Little Endian encoding
   */
  ByteArray &putUint32LE(ByteArray &bytes, uint32_t n);

  /**
   * Put an 64-bit number into the byte array in Big Endian encoding
   */
  ByteArray &putUint64BE(ByteArray &bytes, uint64_t n);

  /**
   * Put a 64-bit number into the byte array in Little Endian encoding
   */
  ByteArray &putUint64LE(ByteArray &bytes, uint64_t n);

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
  struct hash<libp2p::common::ByteArray> {
    size_t operator()(const libp2p::common::ByteArray &x) const;
  };
}  // namespace std

#endif  // LIBP2P_BYTEUTIL_HPP
