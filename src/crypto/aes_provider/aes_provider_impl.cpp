/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/aes_provider/aes_provider_impl.hpp>

#include <openssl/aes.h>  // for AES_BLOCK_SIZE
#include <openssl/evp.h>
#include <gsl/span>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto::aes {

  using libp2p::common::ByteArray;
  using libp2p::crypto::common::Aes128Secret;
  using libp2p::crypto::common::Aes256Secret;

  outcome::result<ByteArray> encrypt(gsl::span<const uint8_t> data,
                                     gsl::span<const uint8_t> key,
                                     gsl::span<const uint8_t> iv,
                                     const EVP_CIPHER *cipher) {
    if (nullptr == cipher) {
      return std::errc::invalid_argument;
    }

    int len = 0;
    int cipher_len = 0;
    const auto *plain_text = data.data();
    const auto plain_len = data.size();

    constexpr auto block_size = AES_BLOCK_SIZE;  // AES_BLOCK_SIZE = 16
    auto required_length = plain_len + block_size - 1;

    std::vector<uint8_t> cipher_text;
    cipher_text.resize(required_length);
    std::fill(cipher_text.begin(), cipher_text.end(), 0);

    // Create and initialise the context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (nullptr == ctx) {
      return OpenSslError::FAILED_INITIALIZE_CONTEXT;
    }

    // clean-up context at exit from function
    auto clean_at_exit = gsl::finally([ctx]() { EVP_CIPHER_CTX_free(ctx); });

    // Initialise the encryption operation.
    if (1 != EVP_EncryptInit_ex(ctx, cipher, nullptr, key.data(), iv.data())) {
      return OpenSslError::FAILED_INITIALIZE_OPERATION;
    }

    // Provide the message to be encrypted, and obtain the encrypted output.
    if (1
        != EVP_EncryptUpdate(ctx, cipher_text.data(), &len, plain_text,
                             plain_len)) {
      return OpenSslError::FAILED_ENCRYPT_UPDATE;
    }

    cipher_len = len;

    // Finalise the encryption.
    auto *write_position = cipher_text.data() + len;  // NOLINT
    if (1 != EVP_EncryptFinal_ex(ctx, write_position, &len)) {
      return OpenSslError::FAILED_ENCRYPT_FINALIZE;
    }

    cipher_len += len;
    cipher_text.resize(cipher_len);

    return ByteArray(std::move(cipher_text));
  }

  outcome::result<ByteArray> decrypt(gsl::span<const uint8_t> data,
                                     gsl::span<const uint8_t> key,
                                     gsl::span<const uint8_t> iv,
                                     const EVP_CIPHER *cipher) {
    if (nullptr == cipher) {
      return std::errc::invalid_argument;
    }

    int len = 0;
    int plain_len = 0;
    const auto *cipher_text = data.data();
    const auto cipher_len = data.size();

    std::vector<uint8_t> plain_text;
    plain_text.resize(cipher_len);

    // Create and initialise the context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (nullptr == ctx) {
      return OpenSslError::FAILED_INITIALIZE_CONTEXT;
    }

    // clean-up context at exit from function
    auto clean_at_exit = gsl::finally([ctx]() { EVP_CIPHER_CTX_free(ctx); });

    // Initialise the decryption operation.
    if (1 != EVP_DecryptInit_ex(ctx, cipher, nullptr, key.data(), iv.data())) {
      return OpenSslError::FAILED_INITIALIZE_OPERATION;
    }

    // Provide the message to be decrypted, and obtain the plaintext output.
    if (1
        != EVP_DecryptUpdate(ctx, plain_text.data(), &len, cipher_text,
                             cipher_len)) {
      return OpenSslError::FAILED_DECRYPT_UPDATE;
    }

    plain_len = len;

    // Finalise the decryption.
    auto *write_position = plain_text.data() + len;  // NOLINT
    if (1 != EVP_DecryptFinal_ex(ctx, write_position, &len)) {
      return OpenSslError::FAILED_DECRYPT_FINALIZE;
    }

    plain_len += len;
    plain_text.resize(plain_len);

    return ByteArray(std::move(plain_text));
  }

  outcome::result<ByteArray> AesProviderImpl::encryptAesCtr128(
      const Aes128Secret &secret, gsl::span<const uint8_t> data) const {
    auto key_span = gsl::make_span(secret.key);
    auto iv_span = gsl::make_span(secret.iv);

    return encrypt(data, key_span, iv_span, EVP_aes_128_ctr());
  }

  outcome::result<ByteArray> AesProviderImpl::encryptAesCtr256(
      const Aes256Secret &secret, gsl::span<const uint8_t> data) const {
    auto key_span = gsl::make_span(secret.key);
    auto iv_span = gsl::make_span(secret.iv);

    return encrypt(data, key_span, iv_span, EVP_aes_256_ctr());
  }

  outcome::result<ByteArray> AesProviderImpl::decryptAesCtr128(
      const Aes128Secret &secret, gsl::span<const uint8_t> data) const {
    auto key_span = gsl::make_span(secret.key);
    auto iv_span = gsl::make_span(secret.iv);

    return decrypt(data, key_span, iv_span, EVP_aes_128_ctr());
  }

  outcome::result<ByteArray> AesProviderImpl::decryptAesCtr256(
      const Aes256Secret &secret, gsl::span<const uint8_t> data) const {
    auto key_span = gsl::make_span(secret.key);
    auto iv_span = gsl::make_span(secret.iv);

    return decrypt(data, key_span, iv_span, EVP_aes_256_ctr());
  }

}  // namespace libp2p::crypto::aes
