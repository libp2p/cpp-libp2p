/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_CHACHAPOLY_IMPL_HPP
#define LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_CHACHAPOLY_IMPL_HPP

#include <openssl/evp.h>
#include <libp2p/crypto/chachapoly.hpp>
#include <libp2p/log/logger.hpp>

namespace libp2p::crypto::chachapoly {

  class ChaCha20Poly1305Impl : public ChaCha20Poly1305 {
   public:
    explicit ChaCha20Poly1305Impl(Key key);

    outcome::result<ByteArray> encrypt(const Nonce &nonce,
                                       gsl::span<const uint8_t> plaintext,
                                       gsl::span<const uint8_t> aad) override;

    outcome::result<ByteArray> decrypt(const Nonce &nonce,
                                       gsl::span<const uint8_t> ciphertext,
                                       gsl::span<const uint8_t> aad) override;

   private:
    const Key key_;
    const EVP_CIPHER *cipher_;
    const int block_size_;
    libp2p::log::Logger log_ = libp2p::log::createLogger("ChaChaPoly");
  };

}  // namespace libp2p::crypto::chachapoly

#endif  // LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_CHACHAPOLY_IMPL_HPP
