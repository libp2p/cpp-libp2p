/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gsl/gsl_util>
#include <libp2p/crypto/chachapoly/chachapoly_impl.hpp>
#include <libp2p/crypto/common_functions.hpp>
#include <libp2p/crypto/error.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto::chachapoly,
                            ChaCha20Poly1305Impl::Error, e) {
  using E = libp2p::crypto::chachapoly::ChaCha20Poly1305Impl::Error;
  switch (e) {
    case E::MESSAGE_AUTHENTICATION_FAILED:
      return "ChaChaPoly message authentication failed.";
    case E::CIPHERTEXT_TOO_LARGE:
      return "Ciphertext is too large.";
    case E::PLAINTEXT_TOO_LARGE:
      return "Plaintext is too large.";
    default:
      return "Unknown failure.";
  }
}

namespace libp2p::crypto::chachapoly {

  ChaCha20Poly1305Impl::ChaCha20Poly1305Impl(const Key &key, Mode mode)
      : key_{key}, mode_{mode} {}

  outcome::result<ByteArray> ChaCha20Poly1305Impl::crypt(
      gsl::span<const uint8_t> data, uint64_t nonce,
      gsl::span<const uint8_t> associated_data) {
    auto nonce_bytes = crypto::asArray<Nonce>(nonce64to12(nonce));
    return crypt(data, nonce_bytes, associated_data);
  }

  outcome::result<ByteArray> ChaCha20Poly1305Impl::crypt(
      gsl::span<const uint8_t> data, Nonce nonce,
      gsl::span<const uint8_t> associated_data) {
    auto ctx = EVP_CIPHER_CTX_new();
    if (nullptr == ctx) {
      return OpenSslError::FAILED_INITIALIZE_CONTEXT;
    }
    auto free_ctx = gsl::finally([ctx] { EVP_CIPHER_CTX_free(ctx); });

    int mode_n{Mode::ENCRYPT == mode_ ? 1 : 0};
    if (1
        != EVP_CipherInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr,
                             nullptr, mode_n)) {
      log_->debug("Failed to initialize cipher.");
      return OpenSslError::FAILED_INITIALIZE_OPERATION;
    }
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, 12, nullptr)) {
      log_->debug("Failed to set nonce length.");
      return OpenSslError::FAILED_INITIALIZE_OPERATION;
    }

    ByteArray out_buffer;
    int out_len{0};

    if (Mode::DECRYPT == mode_) {
      if (data.size() < 16) {
        log_->debug("Ciphertext is too short.");
        return Error::MESSAGE_AUTHENTICATION_FAILED;
      }
      if (static_cast<uint64_t>(data.size()) > ((1ull << 38) - 48)) {
        log_->debug("Ciphertext is too large.");
        return Error::CIPHERTEXT_TOO_LARGE;
      }
      if (1
          != EVP_CIPHER_CTX_ctrl(
              ctx, EVP_CTRL_AEAD_SET_TAG, 16,
              // it is safe to remove const - here we are just reading the tag,
              // but via a common interface which accepts only non-const ptr
              const_cast<uint8_t *>(data.data() + (data.size() - 16)))) {
        log_->debug("Failed to set tag for decryption.");
        return OpenSslError::FAILED_INITIALIZE_OPERATION;
      }
      //      out_buffer.resize(data.size());  // todo
    } else {
      if (static_cast<uint64_t>(data.size()) > ((1ull << 38) - 64)) {
        log_->debug("Plaintext is too large.");
        return Error::PLAINTEXT_TOO_LARGE;
      }
      out_buffer.resize(data.size() + 16);
    }
    std::fill(out_buffer.begin(), out_buffer.end(), 0u);

    if (1
        != EVP_CipherInit_ex(ctx, nullptr, nullptr, key_.data(), nonce.data(),
                             mode_n)) {
      log_->debug("Failed to finalize cipher initialization.");
      return OpenSslError::FAILED_INITIALIZE_OPERATION;
    }

    if (1
        != EVP_CipherUpdate(ctx, nullptr, &out_len, associated_data.data(),
                            associated_data.size())) {
      return Error::MESSAGE_AUTHENTICATION_FAILED;
    }
    log_->trace("Mode: {}crypt, Data size: {}, AAD size: {}, Outlen 1: {}",
                Mode::DECRYPT == mode_ ? "de" : "en", data.size(),
                associated_data.size(), out_len);
    //    out_buffer.resize(
    //        out_len + data.size()
    //        + 64);  // TODO define the correct size, 64 is a random number
    if (1
        != EVP_CipherUpdate(ctx, out_buffer.data(), &out_len, data.data(),
                            data.size())) {
      log_->debug("{}crypt update failed.",
                  Mode::DECRYPT == mode_ ? "De" : "En");
      return Mode::DECRYPT == mode_ ? OpenSslError::FAILED_DECRYPT_UPDATE
                                    : OpenSslError::FAILED_ENCRYPT_UPDATE;
    }
    log_->trace("Outlen 2: {}", out_len);

    if (1 != EVP_CipherFinal_ex(ctx, out_buffer.data() + out_len, &out_len)) {
      log_->debug("{}crypt finalize failed.",
                  Mode::DECRYPT == mode_ ? "De" : "En");
      return Mode::DECRYPT == mode_ ? OpenSslError::FAILED_DECRYPT_FINALIZE
                                    : OpenSslError::FAILED_ENCRYPT_FINALIZE;
    }
    log_->trace("Outlen 3: {}", out_len);
    if (Mode::ENCRYPT == mode_) {
      if (1
          != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16,
                                 out_buffer.data() + data.size())) {
        log_->debug("Failed to write tag during encryption finalization.");
        return OpenSslError::FAILED_ENCRYPT_FINALIZE;
      }
    }
    log_->trace("Out buf size: {}", out_buffer.size());
    return out_buffer;
  }

  template <Mode CipherMode>
  CcpCipher<CipherMode>::CcpCipher(const Key &key, uint64_t starting_nonce)
      : ChaCha20Poly1305Impl(key, CipherMode), nonce{starting_nonce} {}

  template <Mode CipherMode>
  outcome::result<ByteArray> CcpCipher<CipherMode>::operator()(
      gsl::span<const uint8_t> data, gsl::span<const uint8_t> associated_data) {
    auto result = crypt(data, nonce, associated_data);
    ++nonce;
    return result;
  }

  template class CcpCipher<Mode::ENCRYPT>;
  template class CcpCipher<Mode::DECRYPT>;

}  // namespace libp2p::crypto::chachapoly
