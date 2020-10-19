/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gsl/gsl_util>
#include <libp2p/crypto/chachapoly/chachapoly_impl.hpp>
#include <libp2p/crypto/common_functions.hpp>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto::chachapoly {

#define IF1(expr, err, result) \
  if (1 != (expr)) {           \
    log_->error((err));        \
    return (result);           \
  }

  ChaCha20Poly1305Impl::ChaCha20Poly1305Impl(Key key)
      : key_{key},
        cipher_{EVP_chacha20_poly1305()},
        block_size_{EVP_CIPHER_block_size(cipher_)} {}

  outcome::result<ByteArray> ChaCha20Poly1305Impl::encrypt(
      const Nonce &nonce, gsl::span<const uint8_t> plaintext,
      gsl::span<const uint8_t> aad) {
    const auto init_failure = OpenSslError::FAILED_INITIALIZE_OPERATION;
    std::vector<uint8_t> result;
    // just reserving the space, possibly without actual memory allocation:
    // ciphertext length equals to plaintext length
    // one block size for EncryptFinal_ex (usually not required)
    // 16 bytes for the tag
    result.reserve(plaintext.size() + block_size_ + 16);
    auto *ctx = EVP_CIPHER_CTX_new();
    if (nullptr == ctx) {
      return OpenSslError::FAILED_INITIALIZE_CONTEXT;
    }
    auto free_ctx = gsl::finally([ctx] { EVP_CIPHER_CTX_free(ctx); });

    IF1(EVP_EncryptInit_ex(ctx, cipher_, nullptr, nullptr, nullptr),
        "Failed to initialize EVP encryption.", init_failure)

    IF1(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, 12, nullptr),
        "Cannot set AEAD initialization vector length.", init_failure)

    IF1(EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_.data(), nonce.data()),
        "Cannot finalize init of encryption engine.", init_failure)

    int len{0};
    IF1(EVP_EncryptUpdate(ctx, nullptr, &len, nullptr, plaintext.size()),
        "Failed to calculate cipher text length.",
        OpenSslError::FAILED_ENCRYPT_UPDATE)

    // does actual memory allocation
    result.resize(len + block_size_ + 16, 0u);
    if (not aad.empty()) {
      IF1(EVP_EncryptUpdate(ctx, nullptr, &len, aad.data(), aad.size()),
          "Failed to apply additional authentication data during encryption.",
          OpenSslError::FAILED_ENCRYPT_UPDATE)
    }

    IF1(EVP_EncryptUpdate(ctx, result.data(), &len, plaintext.data(),
                          plaintext.size()),
        "Plaintext encryption failed.", OpenSslError::FAILED_ENCRYPT_UPDATE)
    int ciphertext_len = len;  // without tag size
    IF1(EVP_EncryptFinal_ex(ctx, result.data() + len, &len),  // NOLINT
        "Unable to finalize encryption.", OpenSslError::FAILED_ENCRYPT_FINALIZE)

    ciphertext_len += len;
    IF1(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, EVP_CTRL_AEAD_GET_TAG,
                            result.data() + ciphertext_len),  // NOLINT
        "Failed to write tag.", OpenSslError::FAILED_ENCRYPT_FINALIZE)

    // remove the last block_size bytes if those were not used
    result.resize(ciphertext_len + 16);
    return result;
  }

  outcome::result<ByteArray> ChaCha20Poly1305Impl::decrypt(
      const Nonce &nonce, gsl::span<const uint8_t> ciphertext,
      gsl::span<const uint8_t> aad) {
    ByteArray result;
    // plain text should take less bytes than cipher text,
    // at least it would not contain tag-length bytes (16).
    // single block size for the case when len(in) == len(out)
    result.resize(ciphertext.size() + block_size_, 0u);
    auto *ctx = EVP_CIPHER_CTX_new();
    auto free_ctx = gsl::finally([ctx] { EVP_CIPHER_CTX_free(ctx); });

    IF1(EVP_DecryptInit_ex(ctx, cipher_, nullptr, nullptr, nullptr),
        "Failed to initialize decryption engine.",
        OpenSslError::FAILED_INITIALIZE_OPERATION)

    IF1(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, 12, nullptr),
        "Cannot set AEAD initialization vector length.",
        OpenSslError::FAILED_INITIALIZE_OPERATION)

    IF1(EVP_CIPHER_CTX_ctrl(
            ctx, EVP_CTRL_AEAD_SET_TAG, EVP_CTRL_AEAD_GET_TAG,
            (uint8_t *)ciphertext.data() + ciphertext.size() - 16),  // NOLINT
        "Failed to specify buffer for further tag reading.",
        OpenSslError::FAILED_DECRYPT_UPDATE)

    IF1(EVP_DecryptInit_ex(ctx, nullptr, nullptr, key_.data(), nonce.data()),
        "Cannot finalize init of decryption engine.",
        OpenSslError::FAILED_INITIALIZE_OPERATION)

    int len{0};
    if (not aad.empty()) {
      IF1(EVP_DecryptUpdate(ctx, nullptr, &len, aad.data(), aad.size()),
          "Failed to apply additional authentication data during decryption.",
          OpenSslError::FAILED_DECRYPT_UPDATE)
    }

    IF1(EVP_DecryptUpdate(ctx, result.data(), &len, ciphertext.data(),
                          ciphertext.size() - 16),
        "Ciphertext decryption failed.", OpenSslError::FAILED_DECRYPT_UPDATE)

    IF1(EVP_DecryptFinal_ex(ctx,
                            result.data() + len,  // NOLINT
                            &len),
        "Failed to finalize decryption.",
        OpenSslError::FAILED_DECRYPT_FINALIZE);

    result.resize(ciphertext.size() - 16);
    return result;
  }

}  // namespace libp2p::crypto::chachapoly
