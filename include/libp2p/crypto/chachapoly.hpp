/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <span>

namespace libp2p::crypto::chachapoly {
  using libp2p::Bytes;
  using Key = std::array<uint8_t, 32>;
  using Nonce = std::array<uint8_t, 12>;

  class ChaCha20Poly1305 {
   public:
    virtual ~ChaCha20Poly1305() = default;

    /**
     * Does authenticated encryption with associated data (AEAD)
     * @param plaintext to cipher
     * @param nonce - custom specified nonce bytes
     * @param associated_data - data for message authentication
     * @return ciphertext bytes
     */
    virtual outcome::result<Bytes> encrypt(const Nonce &nonce,
                                           BytesIn plaintext,
                                           BytesIn aad) = 0;

    /**
     * Does authenticated decryption with associated data (AEAD)
     * @param ciphertext bytes to decrypt
     * @param nonce - custom specified nonce bytes
     * @param associated_data - data for message authentication
     * @return plaintext bytes
     */
    virtual outcome::result<Bytes> decrypt(const Nonce &nonce,
                                           BytesIn ciphertext,
                                           BytesIn aad) = 0;

    /**
     * Convert 64-bit integer to 12-bit long byte sequence with four zero bytes
     * at the beginning
     * @param n - an integer to convert
     * @return - bytes vector
     */
    inline Nonce uint64toNonce(uint64_t n) const {
      Bytes result(4, 0);
      result.reserve(12);
      libp2p::common::putUint64LE(result, n);
      Nonce nonce;
      std::copy_n(result.begin(), nonce.size(), nonce.begin());
      return nonce;
    }
  };

}  // namespace libp2p::crypto::chachapoly
