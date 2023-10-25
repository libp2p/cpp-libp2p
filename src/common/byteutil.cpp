/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/byteutil.hpp>

#include <boost/container_hash/hash.hpp>

namespace libp2p::common {
  Bytes &putUint8(Bytes &bytes, uint8_t n) {
    bytes.push_back(n);
    return bytes;
  }

  Bytes &putUint16BE(Bytes &bytes, uint16_t n) {
    bytes.push_back(static_cast<unsigned char &&>((n >> 8u) & 0xFF));
    return putUint8(bytes, n);
  }

  Bytes &putUint16LE(Bytes &bytes, uint16_t n) {
    putUint8(bytes, n);
    bytes.push_back(static_cast<unsigned char &&>((n >> 8u) & 0xFF));
    return bytes;
  }

  Bytes &putUint32BE(Bytes &bytes, uint32_t n) {
    bytes.push_back(static_cast<unsigned char &&>((n >> 24u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 16u) & 0xFF));
    return putUint16BE(bytes, n);
  }

  Bytes &putUint32LE(Bytes &bytes, uint32_t n) {
    putUint16LE(bytes, n);
    bytes.push_back(static_cast<unsigned char &&>((n >> 16u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 24u) & 0xFF));
    return bytes;
  }

  Bytes &putUint64BE(Bytes &bytes, uint64_t n) {
    bytes.push_back(static_cast<unsigned char &&>((n >> 56u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 48u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 40u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 32u) & 0xFF));
    return putUint32BE(bytes, n);
  }

  Bytes &putUint64LE(Bytes &bytes, uint64_t n) {
    putUint32LE(bytes, n);
    bytes.push_back(static_cast<unsigned char &&>((n >> 32u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 40u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 48u) & 0xFF));
    bytes.push_back(static_cast<unsigned char &&>((n >> 56u) & 0xFF));
    return bytes;
  }
}  // namespace libp2p::common

size_t std::hash<libp2p::Bytes>::operator()(const libp2p::Bytes &x) const {
  return boost::hash_range(x.begin(), x.end());
}
