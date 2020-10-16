/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_HPP
#define LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_HPP

#include <array>

#include <gsl/span>
#include <libp2p/common/byteutil.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::chachapoly {
  using libp2p::common::ByteArray;
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
    virtual outcome::result<ByteArray> encrypt(
        const Nonce &nonce, gsl::span<const uint8_t> plaintext,
        gsl::span<const uint8_t> aad) = 0;

    /**
     * Does authenticated decryption with associated data (AEAD)
     * @param ciphertext bytes to decrypt
     * @param nonce - custom specified nonce bytes
     * @param associated_data - data for message authentication
     * @return plaintext bytes
     */
    virtual outcome::result<ByteArray> decrypt(
        const Nonce &nonce, gsl::span<const uint8_t> ciphertext,
        gsl::span<const uint8_t> aad) = 0;

    /**
     * Convert 64-bit integer to 12-bit long byte sequence with four zero bytes
     * at the beginning
     * @param n - an integer to convert
     * @return - bytes vector
     */
    inline Nonce uint64toNonce(uint64_t n) const {
      ByteArray result(4, 0);
      result.reserve(12);
      libp2p::common::putUint64LE(result, n);
      Nonce nonce;
      std::copy_n(result.begin(), nonce.size(), nonce.begin());
      return nonce;
    }
  };

}  // namespace libp2p::crypto::chachapoly

#endif  // LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_HPP
