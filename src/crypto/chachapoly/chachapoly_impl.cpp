/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openssl/aead.h>
#include <libp2p/crypto/chachapoly/chachapoly_impl.hpp>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto::chachapoly {
  constexpr size_t kTagSize = 16;

#define IF1(expr, err, result) \
  if (1 != (expr)) {           \
    log_->error((err));        \
    return (result);           \
  }

  ChaCha20Poly1305Impl::ChaCha20Poly1305Impl(Key key)
      : key_{key},
        aead_{EVP_aead_chacha20_poly1305()},
        overhead_{EVP_AEAD_max_overhead(aead_)} {}

  outcome::result<void> ChaCha20Poly1305Impl::init(EVP_AEAD_CTX *ctx,
                                                   bool encrypt) {
    IF1(EVP_AEAD_CTX_init_with_direction(
            ctx,
            aead_,
            key_.data(),
            key_.size(),
            kTagSize,
            encrypt ? evp_aead_seal : evp_aead_open),
        "EVP_AEAD_CTX_init",
        OpenSslError::FAILED_INITIALIZE_CONTEXT);
    return outcome::success();
  }

  outcome::result<Bytes> ChaCha20Poly1305Impl::encrypt(const Nonce &nonce,
                                                       BytesIn plaintext,
                                                       BytesIn aad) {
    bssl::ScopedEVP_AEAD_CTX ctx;
    OUTCOME_TRY(init(ctx.get(), true));
    Bytes result;
    // just reserving the space, possibly without actual memory allocation:
    // ciphertext length equals to plaintext length
    // one block size for EncryptFinal_ex (usually not required)
    // 16 bytes for the tag
    result.resize(plaintext.size() + overhead_);
    size_t out_size = 0;
    IF1(EVP_AEAD_CTX_seal(ctx.get(),
                          result.data(),
                          &out_size,
                          result.size(),
                          nonce.data(),
                          nonce.size(),
                          plaintext.data(),
                          plaintext.size(),
                          aad.data(),
                          aad.size()),
        "EVP_AEAD_CTX_seal",
        OpenSslError::FAILED_ENCRYPT_UPDATE);
    result.resize(out_size);
    return result;
  }

  outcome::result<Bytes> ChaCha20Poly1305Impl::decrypt(const Nonce &nonce,
                                                       BytesIn ciphertext,
                                                       BytesIn aad) {
    bssl::ScopedEVP_AEAD_CTX ctx;
    OUTCOME_TRY(init(ctx.get(), false));
    Bytes result;
    // plain text should take less bytes than cipher text,
    // at least it would not contain tag-length bytes (16).
    // single block size for the case when len(in) == len(out)
    result.resize(ciphertext.size());
    size_t out_size = 0;
    IF1(EVP_AEAD_CTX_open(ctx.get(),
                          result.data(),
                          &out_size,
                          result.size(),
                          nonce.data(),
                          nonce.size(),
                          ciphertext.data(),
                          ciphertext.size(),
                          aad.data(),
                          aad.size()),
        "EVP_AEAD_CTX_open",
        OpenSslError::FAILED_DECRYPT_UPDATE);
    result.resize(out_size);
    return result;
  }

}  // namespace libp2p::crypto::chachapoly
