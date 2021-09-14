/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>

#include <libp2p/crypto/hmac_provider/hmac_provider_ctr_impl.hpp>

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <gsl/gsl_util>
#include <gsl/span>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto::hmac {
  using ByteArray = libp2p::common::ByteArray;

  HmacProviderCtrImpl::HmacProviderCtrImpl(HashType hash_type,
                                           gsl::span<const uint8_t> key)
      : hash_type_{hash_type},
        key_(key.begin(), key.end()),
        hash_st_{hash_type_ == HashType::SHA1
                     ? EVP_sha1()
                     : (hash_type_ == HashType::SHA256 ? EVP_sha256()
                                                       : EVP_sha512())},
        hmac_ctx_{nullptr},
        initialized_{false} {
    switch (hash_type_) {
      case common::HashType::SHA1:
      case common::HashType::SHA256:
      case common::HashType::SHA512:
        initialized_ = true;
        break;
      default:
        return;
    }
    if (nullptr == (hmac_ctx_ = HMAC_CTX_new())) {
      initialized_ = false;
      return;
    }
    initialized_ = 1
        == HMAC_Init_ex(hmac_ctx_, key_.data(), static_cast<int>(key_.size()),
                        hash_st_, nullptr);
  }

  HmacProviderCtrImpl::~HmacProviderCtrImpl() {
    sinkCtx(HmacProviderCtrImpl::digestSize());
  }

  outcome::result<void> HmacProviderCtrImpl::write(
      gsl::span<const uint8_t> data) {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (1 != HMAC_Update(hmac_ctx_, data.data(), data.size())) {
      return HmacProviderError::FAILED_UPDATE_DIGEST;
    }
    return outcome::success();
  }

  outcome::result<void> HmacProviderCtrImpl::digestOut(
      gsl::span<uint8_t> out) const {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (out.size() != static_cast<ptrdiff_t>(digestSize())) {
      return HmacProviderError::WRONG_DIGEST_SIZE;
    }
    HMAC_CTX *ctx_copy = HMAC_CTX_new();
    if (nullptr == ctx_copy) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (1 != HMAC_CTX_copy(ctx_copy, hmac_ctx_)) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    auto free_ctx_copy = gsl::finally([ctx_copy] { HMAC_CTX_free(ctx_copy); });
    unsigned len{0};
    if (1 != HMAC_Final(ctx_copy, out.data(), &len)) {
      return HmacProviderError::FAILED_FINALIZE_DIGEST;
    }
    if (len != digestSize()) {
      return HmacProviderError::WRONG_DIGEST_SIZE;
    }
    return outcome::success();
  }

  outcome::result<void> HmacProviderCtrImpl::reset() {
    sinkCtx(digestSize());
    hmac_ctx_ = HMAC_CTX_new();
    if (nullptr == hmac_ctx_
        or 1
            != HMAC_Init_ex(hmac_ctx_, key_.data(),
                            static_cast<int>(key_.size()), hash_st_, nullptr)) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    initialized_ = true;
    return outcome::success();
  }

  size_t HmacProviderCtrImpl::digestSize() const {
    if (initialized_) {
      return EVP_MD_size(hash_st_);
    }
    return 0;
  }

  size_t HmacProviderCtrImpl::blockSize() const {
    if (initialized_) {
      return EVP_MD_block_size(hash_st_);
    }
    return 0;
  }

  void HmacProviderCtrImpl::sinkCtx(size_t digest_size) {
    if (initialized_) {
      std::vector<uint8_t> data;
      data.resize(digest_size);
      unsigned len{0};
      HMAC_Final(hmac_ctx_, data.data(), &len);
      HMAC_CTX_free(hmac_ctx_);
      hmac_ctx_ = nullptr;
      memset(data.data(), 0, data.size());
      initialized_ = false;
    }
  }

  HashType HmacProviderCtrImpl::hashType() const {
    return hash_type_;
  }
}  // namespace libp2p::crypto::hmac
