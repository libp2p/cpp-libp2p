/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_HASH_TYPE_HPP
#define LIBP2P_HASH_TYPE_HPP

namespace libp2p::multi {
  /// https://github.com/multiformats/js-multihash/blob/master/src/constants.js
  enum HashType : uint64_t {
    identity = 0x0,
    sha1 = 0x11,
    sha256 = 0x12,
    sha512 = 0x13,
    blake2s128 = 0xb250,
    blake2s256 = 0xb260
  };
}  // namespace libp2p::multi

#endif  // LIBP2P_HASH_TYPE_HPP
