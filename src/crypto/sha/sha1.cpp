/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>

#include <libp2p/crypto/sha/sha1.hpp>

#include <openssl/sha.h>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto {
  Sha1::Sha1() : initialized_(1 == SHA1_Init(&ctx_)) {}  // NOLINT

  Sha1::~Sha1() {
    sinkCtx();
  }

  outcome::result<void> Sha1::write(gsl::span<const uint8_t> data) {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (1 != SHA1_Update(&ctx_, data.data(), data.size())) {
      return HmacProviderError::FAILED_UPDATE_DIGEST;
    }
    return outcome::success();
  }

  outcome::result<void> Sha1::digestOut(gsl::span<uint8_t> out) const {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (out.size() != static_cast<ptrdiff_t>(digestSize())) {
      return HmacProviderError::WRONG_DIGEST_SIZE;
    }
    SHA_CTX ctx = ctx_;
    if (1 != SHA1_Final(out.data(), &ctx)) {
      return HmacProviderError::FAILED_FINALIZE_DIGEST;
    }
    return outcome::success();
  }

  outcome::result<void> Sha1::reset() {
    sinkCtx();
    if (1 != SHA1_Init(&ctx_)) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    initialized_ = true;
    return outcome::success();
  }

  size_t Sha1::digestSize() const {
    return SHA_DIGEST_LENGTH;
  }

  size_t Sha1::blockSize() const {
    return SHA_CBLOCK;
  }

  void Sha1::sinkCtx() {
    if (initialized_) {
      libp2p::common::Hash256 digest;
      SHA1_Final(digest.data(), &ctx_);
      memset(digest.data(), 0, digest.size());
      initialized_ = false;
    }
  }

  HashType Sha1::hashType() const {
    return HashType::SHA1;
  }

  outcome::result<libp2p::common::Hash160> sha1(
      gsl::span<const uint8_t> input) {
    Sha1 sha;
    OUTCOME_TRY(sha.write(input));
    outcome::result<libp2p::common::Hash160> result{outcome::success()};
    OUTCOME_TRY(sha.digestOut(result.value()));
    return result;
  }
}  // namespace libp2p::crypto
