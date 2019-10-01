/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BYTEUTIL_HPP
#define LIBP2P_BYTEUTIL_HPP

#include <cstdint>
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
   * Put an 32-bit number into the byte array in Big Endian encoding
   */
  ByteArray &putUint32BE(ByteArray &bytes, uint32_t n);

  /**
   * Put an 64-bit number into the byte array in Big Endian encoding
   */
  ByteArray &putUin64BE(ByteArray &bytes, uint64_t n);
}  // namespace libp2p::common

#endif  // LIBP2P_BYTEUTIL_HPP
