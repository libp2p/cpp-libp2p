/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_HASH_TYPE_HPP
#define LIBP2P_HASH_TYPE_HPP

namespace libp2p::multi {
  /// TODO(Harrm) FIL-14: Hash types are a part of multicodec table, it would be
  /// good to move them there to avoid duplication and allow for extraction of
  /// human-friendly name of a type from its code
  /// @see MulticodecType
  /// https://github.com/multiformats/js-multihash/blob/master/src/constants.js
  enum HashType : uint64_t {
    identity = 0x0,
    sha1 = 0x11,
    sha256 = 0x12,
    sha512 = 0x13,
    blake2b_256 = 0xb220,
    blake2s128 = 0xb250,
    blake2s256 = 0xb260,
    sha2_256_trunc254_padded = 0x1012,
    poseidon_bls12_381_a1_fc1 = 0xb401,
  };
}  // namespace libp2p::multi

#endif  // LIBP2P_HASH_TYPE_HPP
