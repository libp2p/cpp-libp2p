/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP
#define LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP

#include <libp2p/crypto/aes_ctr.hpp>

#include <openssl/evp.h>
#include <gsl/span>
#include <libp2p/crypto/common.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::aes {

  class AesCtrImpl : public AesCtr {
   public:
    using Aes128Secret = libp2p::crypto::common::Aes128Secret;
    using Aes256Secret = libp2p::crypto::common::Aes256Secret;

    enum class Mode {
      // all the constants are explicitly specified in accordance to OpenSSL
      // values
      DECRYPT = 0,
      ENCRYPT = 1,
    };

    AesCtrImpl(const Aes128Secret &secret, Mode mode);

    AesCtrImpl(const Aes256Secret &secret, Mode mode);

    ~AesCtrImpl() override;

    outcome::result<ByteArray> crypt(
        gsl::span<const uint8_t> data) const override;

    outcome::result<ByteArray> finalize() override;

   private:
    outcome::result<void> init(gsl::span<const uint8_t> key,
                               gsl::span<const uint8_t> iv,
                               const EVP_CIPHER *cipher);

    const Mode mode_;
    outcome::result<void> initialization_error_{outcome::success()};
    EVP_CIPHER_CTX *ctx_{nullptr};
  };

}  // namespace libp2p::crypto::aes

#endif  // LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP
