/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>

#include <openssl/sha.h>
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/sha/sha512.hpp>

namespace libp2p::crypto {

  Sha512::Sha512() {  // NOLINT
    initialized_ = 1 == SHA512_Init(&ctx_);
  }

  Sha512::~Sha512() {
    sinkCtx();
  }

  outcome::result<void> Sha512::write(gsl::span<const uint8_t> data) {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (1 != SHA512_Update(&ctx_, data.data(), data.size())) {
      return HmacProviderError::FAILED_UPDATE_DIGEST;
    }
    return outcome::success();
  }

  outcome::result<void> Sha512::digestOut(gsl::span<uint8_t> out) const {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (out.size() != static_cast<ptrdiff_t>(digestSize())) {
      return HmacProviderError::WRONG_DIGEST_SIZE;
    }
    SHA512_CTX ctx = ctx_;
    if (1 != SHA512_Final(out.data(), &ctx)) {
      return HmacProviderError::FAILED_FINALIZE_DIGEST;
    }
    return outcome::success();
  }

  outcome::result<void> Sha512::reset() {
    sinkCtx();
    if (1 != SHA512_Init(&ctx_)) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    initialized_ = true;
    return outcome::success();
  }

  size_t Sha512::digestSize() const {
    return SHA512_DIGEST_LENGTH;
  }

  size_t Sha512::blockSize() const {
    return SHA512_CBLOCK;
  }

  void Sha512::sinkCtx() {
    if (initialized_) {
      libp2p::common::Hash512 digest;
      SHA512_Final(digest.data(), &ctx_);
      memset(digest.data(), 0, digest.size());
      initialized_ = false;
    }
  }

  HashType Sha512::hashType() const {
    return HashType::SHA512;
  }

  outcome::result<libp2p::common::Hash512> sha512(
      gsl::span<const uint8_t> input) {
    Sha512 sha;
    OUTCOME_TRY(sha.write(input));
    outcome::result<libp2p::common::Hash512> result{outcome::success()};
    OUTCOME_TRY(sha.digestOut(result.value()));
    return result;
  }
}  // namespace libp2p::crypto
