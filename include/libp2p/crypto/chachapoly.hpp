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

  enum class Mode {
    // the constants are the same as in OpenSSL
    DECRYPT = 0,
    ENCRYPT = 1,
  };

  using libp2p::common::ByteArray;
  using Key = std::array<uint8_t, 32>;
  using Nonce = std::array<uint8_t, 12>;

  class ChaCha20Poly1305 {
   public:
    virtual ~ChaCha20Poly1305() = default;

    /*
     * NB:
     * Descendants are to implement a constructor or other method to set the
     * mode which crypt method should perform (encrypt or decrypt) as well as
     * the key setup.
     */

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
     * Performs encryption or decryption. Operation depends on initialization of
     * cipher implementing class.
     *
     * Nonce is automatically calculated from continuously incrementing uint64_t
     * number after each call of the method.
     *
     * First encryption or decryption will be performed with the nonce equal to
     * 0 (zero).
     *
     * @param data - bytes to process
     * @param aad - data for message authentication
     * @return
     */
    virtual outcome::result<ByteArray> crypt(gsl::span<const uint8_t> data,
                                             gsl::span<const uint8_t> aad) = 0;

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
