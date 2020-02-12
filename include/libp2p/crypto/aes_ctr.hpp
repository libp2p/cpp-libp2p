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
   * An interface for stream data cipher encryption or decryption engine with
   * AES128 or AES256 algorithms.
   */
  class AesCtr {
   public:
    using ByteArray = libp2p::common::ByteArray;

    virtual ~AesCtr() = default;

    /**
     * Encrypts or decrypts user data
     * @param data to be processed
     * @return processed data chunk or an error
     */
    virtual outcome::result<ByteArray> crypt(
        gsl::span<const uint8_t> data) const = 0;

    /**
     * Does stream data finalization
     * @return bytes buffer to correctly pad all the previously processed
     * data or an error
     */
    virtual outcome::result<ByteArray> finalize() = 0;
  };
}  // namespace libp2p::crypto::aes

#endif  // LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP
