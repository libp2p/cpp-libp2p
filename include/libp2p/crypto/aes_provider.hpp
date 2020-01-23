/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP
#define LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP

#include <gsl/span>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::aes {
  /**
   * @class AesCrypt provides methods for encryption and decryption by means of
   * AES128 and AES256
   */
  class AesProvider {
    using ByteArray = libp2p::common::ByteArray;
    using Aes128Secret = libp2p::crypto::common::Aes128Secret;
    using Aes256Secret = libp2p::crypto::common::Aes256Secret;

   public:
    virtual ~AesProvider() = default;

    /**
     * @brief encrypts data using AES128 cipher
     * @param secret cipher secret
     * @param data plain data
     * @return encrypted data or error
     */
    virtual outcome::result<ByteArray> encryptAesCtr128(
        const Aes128Secret &secret, gsl::span<const uint8_t> data) const = 0;

    /**
     * @brief decrypts data using AES128 cipher
     * @param secret cipher secret
     * @param data encrypted data
     * @return decrypted data or error
     */
    virtual outcome::result<ByteArray> decryptAesCtr128(
        const Aes128Secret &secret, gsl::span<const uint8_t> data) const = 0;

    /**
     * @brief encrypts data using AES256 cipher
     * @param secret cipher secret
     * @param data plain data
     * @return encrypted data or error
     */
    virtual outcome::result<ByteArray> encryptAesCtr256(
        const Aes256Secret &secret, gsl::span<const uint8_t> data) const = 0;

    /**œ
     * @brief decrypts data using AES256 cipher
     * @param secret cipher secret
     * @param data encrypted data
     * @return decrypted data or error
     */
    virtual outcome::result<ByteArray> decryptAesCtr256(
        const Aes256Secret &secret, gsl::span<const uint8_t> data) const = 0;
  };
}  // namespace libp2p::crypto::aes

#endif  // LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP
