/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>

#include <libp2p/crypto/sha/sha256.hpp>

#include <openssl/sha.h>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto {
  Sha256::Sha256() {  // NOLINT
    initialized_ = 1 == SHA256_Init(&ctx_);
  }

  Sha256::~Sha256() {
    sinkCtx();
  }

  outcome::result<void> Sha256::write(gsl::span<const uint8_t> data) {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (1 != SHA256_Update(&ctx_, data.data(), data.size())) {
      return HmacProviderError::FAILED_UPDATE_DIGEST;
    }
    return outcome::success();
  }

  outcome::result<void> Sha256::digestOut(gsl::span<uint8_t> out) const {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (out.size() != static_cast<ptrdiff_t>(digestSize())) {
      return HmacProviderError::WRONG_DIGEST_SIZE;
    }
    SHA256_CTX ctx = ctx_;
    if (1 != SHA256_Final(out.data(), &ctx)) {
      return HmacProviderError::FAILED_FINALIZE_DIGEST;
    }
    return outcome::success();
  }

  outcome::result<void> Sha256::reset() {
    sinkCtx();
    if (1 != SHA256_Init(&ctx_)) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    initialized_ = true;
    return outcome::success();
  }

  size_t Sha256::digestSize() const {
    return SHA256_DIGEST_LENGTH;
  }

  size_t Sha256::blockSize() const {
    return SHA256_CBLOCK;
  }

  void Sha256::sinkCtx() {
    if (initialized_) {
      libp2p::common::Hash256 digest;
      SHA256_Final(digest.data(), &ctx_);
      memset(digest.data(), 0, digest.size());
      initialized_ = false;
    }
  }

  HashType Sha256::hashType() const {
    return HashType::SHA256;
  }

  outcome::result<libp2p::common::Hash256> sha256(
      gsl::span<const uint8_t> input) {
    Sha256 sha;
    OUTCOME_TRY(sha.write(input));
    outcome::result<libp2p::common::Hash256> result{outcome::success()};
    OUTCOME_TRY(sha.digestOut(result.value()));
    return result;
  }
}  // namespace libp2p::crypto
