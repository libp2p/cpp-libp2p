/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_CHACHAPOLY_IMPL_HPP
#define LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_CHACHAPOLY_IMPL_HPP

#include <openssl/evp.h>
#include <libp2p/common/logger.hpp>
#include <libp2p/crypto/chachapoly.hpp>

namespace libp2p::crypto::chachapoly {

  enum class Mode {
    // all the constants are explicitly specified in accordance to OpenSSL
    // values
    DECRYPT = 0,
    ENCRYPT = 1,
  };

  class ChaCha20Poly1305Impl : public ChaCha20Poly1305 {
   public:
    enum class Error {
      MESSAGE_AUTHENTICATION_FAILED = 1,
      CIPHERTEXT_TOO_LARGE,
      PLAINTEXT_TOO_LARGE,
    };

    ChaCha20Poly1305Impl(const Key &key, Mode mode);

    outcome::result<ByteArray> seal(gsl::span<const uint)

    outcome::result<ByteArray> crypt(
        gsl::span<const uint8_t> data, uint64_t nonce,
        gsl::span<const uint8_t> associated_data) override;

    outcome::result<ByteArray> crypt(
        gsl::span<const uint8_t> data, Nonce nonce,
        gsl::span<const uint8_t> associated_data) override;

   private:
    const Key key_;
    const Mode mode_;
    libp2p::common::Logger log_ =
        libp2p::common::createLogger("ChaChaPolyImpl");
  };

  template <Mode CipherMode>
  class CcpCipher : public ChaCha20Poly1305Impl {
   public:
    CcpCipher(const Key &key, uint64_t starting_nonce = 0);

    outcome::result<ByteArray> operator()(
        gsl::span<const uint8_t> data,
        gsl::span<const uint8_t> associated_data);

   private:
    uint64_t nonce;
  };

  // explicit templates instantiation lays in cpp
  using CcpEncryptor = CcpCipher<Mode::ENCRYPT>;
  using CcpDecryptor = CcpCipher<Mode::DECRYPT>;

}  // namespace libp2p::crypto::chachapoly

OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto::chachapoly,
                          ChaCha20Poly1305Impl::Error);

#endif  // LIBP2P_INCLUDE_LIBP2P_CRYPTO_CHACHAPOLY_CHACHAPOLY_IMPL_HPP
