/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/byteutil.hpp>

#include <boost/container_hash/hash.hpp>

namespace libp2p::common {
  ByteArray &putUint8(ByteArray &bytes, uint8_t n) {
    bytes.push_back(n);
    return bytes;
  }

  ByteArray &putUint16BE(ByteArray &bytes, uint16_t n) {
    bytes.push_back(static_cast<unsigned char &&>((n >> 8u) & 0xFF));
    return putUint8(bytes, n);
  }

  ByteArray &putUint16LE(ByteArray &bytes, uint16_t n) {
    putUint8(bytes, n);
    bytes.push_back(static_cast<unsigned char &&>((n >> 8u) & 0xFF));
    return bytes;
  }

  ByteArray &putUint32BE(ByteArray &bytes, uint32_t n) {
    bytes.push_back(static_cast<unsigned char &&>((n >> 24u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 16u) & 0xFF));
    return putUint16BE(bytes, n);
  }

  ByteArray &putUint32LE(ByteArray &bytes, uint32_t n) {
    putUint16LE(bytes, n);
    bytes.push_back(static_cast<unsigned char &&>((n >> 16u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 24u) & 0xFF));
    return bytes;
  }

  ByteArray &putUint64BE(ByteArray &bytes, uint64_t n) {
    bytes.push_back(static_cast<unsigned char &&>((n >> 56u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 48u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 40u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 32u) & 0xFF));
    return putUint32BE(bytes, n);
  }

  ByteArray &putUint64LE(ByteArray &bytes, uint64_t n) {
    putUint32LE(bytes, n);
    bytes.push_back(static_cast<unsigned char &&>((n >> 32u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 40u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 48u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 56u) & 0xFF));
    return bytes;
  }
}  // namespace libp2p::common

size_t std::hash<libp2p::common::ByteArray>::operator()(
    const libp2p::common::ByteArray &x) const {
  return boost::hash_range(x.begin(), x.end());
}
