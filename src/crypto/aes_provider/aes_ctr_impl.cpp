/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/aes_ctr/aes_ctr_impl.hpp>

#include <openssl/aes.h>  // for AES_BLOCK_SIZE
#include <openssl/evp.h>

#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto::aes {

  using libp2p::Bytes;
  using libp2p::crypto::common::Aes128Secret;
  using libp2p::crypto::common::Aes256Secret;

  AesCtrImpl::AesCtrImpl(const Aes128Secret &secret, AesCtrImpl::Mode mode)
      : mode_{mode} {
    initialization_error_ = init(secret.key, secret.iv, EVP_aes_128_ctr());
  }

  AesCtrImpl::AesCtrImpl(const Aes256Secret &secret, AesCtrImpl::Mode mode)
      : mode_{mode} {
    initialization_error_ = init(secret.key, secret.iv, EVP_aes_256_ctr());
  }

  AesCtrImpl::~AesCtrImpl() {
    if (nullptr != ctx_) {
      EVP_CIPHER_CTX_free(ctx_);
      ctx_ = nullptr;
    }
  }

  outcome::result<void> AesCtrImpl::init(BytesIn key,
                                         BytesIn iv,
                                         const EVP_CIPHER *cipher) {
    ctx_ = EVP_CIPHER_CTX_new();
    if (nullptr == ctx_) {
      return OpenSslError::FAILED_INITIALIZE_CONTEXT;
    }

    int mode{Mode::ENCRYPT == mode_ ? 1 : 0};
    if (1
        != EVP_CipherInit_ex(
            ctx_, cipher, nullptr, key.data(), iv.data(), mode)) {
      return OpenSslError::FAILED_INITIALIZE_OPERATION;
    }

    return outcome::success();
  }

  outcome::result<Bytes> AesCtrImpl::crypt(BytesIn data) const {
    if (initialization_error_.has_error()) {
      return initialization_error_.error();
    }

    Bytes out_buffer;
    out_buffer.resize(data.size() + AES_BLOCK_SIZE);
    std::fill(out_buffer.begin(), out_buffer.end(), 0u);
    int out_len{0};

    if (1
        != EVP_CipherUpdate(
            ctx_, out_buffer.data(), &out_len, data.data(), data.size())) {
      switch (mode_) {
        case Mode::ENCRYPT:
          return OpenSslError::FAILED_ENCRYPT_UPDATE;
        case Mode::DECRYPT:
          return OpenSslError::FAILED_DECRYPT_UPDATE;
      }
    }
    out_buffer.resize(out_len);
    return out_buffer;
  }

  outcome::result<Bytes> AesCtrImpl::finalize() {
    if (initialization_error_.has_error()) {
      return initialization_error_.error();
    }
    initialization_error_ = OpenSslError::STREAM_FINALIZED;

    Bytes out_buffer;
    out_buffer.resize(AES_BLOCK_SIZE);
    std::fill(out_buffer.begin(), out_buffer.end(), 0u);
    int out_len{0};
    if (1 != EVP_CipherFinal_ex(ctx_, out_buffer.data(), &out_len)) {
      switch (mode_) {
        case Mode::ENCRYPT:
          return OpenSslError::FAILED_ENCRYPT_FINALIZE;
        case Mode::DECRYPT:
          return OpenSslError::FAILED_DECRYPT_FINALIZE;
      }
    }

    out_buffer.resize(out_len);
    return Bytes(std::move(out_buffer));
  }

}  // namespace libp2p::crypto::aes
