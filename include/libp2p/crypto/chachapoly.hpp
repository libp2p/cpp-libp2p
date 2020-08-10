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
     * Does encryption or decryption via authenticated encryption with
     * associated data (AEAD)
     * @param data - plaintext or ciphertext depending on used mode during class
     * instantiation
     * @param nonce
     * @param associated_data
     * @return encrypted or decrypted data
     */
    virtual outcome::result<ByteArray> crypt(
        gsl::span<const uint8_t> data, uint64_t nonce,
        gsl::span<const uint8_t> associated_data) = 0;

    /**
     * Does encryption or decryption via authenticated encryption with
     * associated data (AEAD)
     * @param data - plaintext or ciphertext depending on used mode during class
     * instantiation
     * @param nonce
     * @param associated_data
     * @return encrypted or decrypted data
     */
    virtual outcome::result<ByteArray> crypt(
        gsl::span<const uint8_t> data, Nonce nonce,
        gsl::span<const uint8_t> associated_data) = 0;

   protected:
    /**
     * Convert 64-bit integer to 12-bit long byte sequence with four zero bytes
     * at the beginning
     * @param n - an integer to convert
     * @return - bytes vector
     */
    inline ByteArray nonce64to12(uint64_t n) const {
      ByteArray result{4, 0};
      result.reserve(12);
      libp2p::common::putUint64LE(result, n);
      return result;
    }
  };

}  // namespace libp2p::crypto::chachapoly

#endif  // LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_HPP
