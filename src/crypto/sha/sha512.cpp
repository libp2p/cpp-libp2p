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

  outcome::result<std::vector<uint8_t>> Sha512::digest() {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    SHA512_CTX ctx = ctx_;
    std::vector<uint8_t> result;
    result.resize(digestSize());
    if (1 != SHA512_Final(result.data(), &ctx)) {
      return HmacProviderError::FAILED_FINALIZE_DIGEST;
    }
    return result;
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

  libp2p::common::Hash512 sha512(std::string_view input) {
    std::vector<uint8_t> bytes{input.begin(), input.end()};
    return sha512(bytes);
  }

  libp2p::common::Hash512 sha512(gsl::span<const uint8_t> input) {
    libp2p::common::Hash512 out;
    SHA512_CTX ctx;
    SHA512_Init(&ctx);
    SHA512_Update(&ctx, input.data(), input.size());
    SHA512_Final(out.data(), &ctx);
    // TODO(igor-egorov) FIL-67 Try to add checks for SHA-X return values
    return out;
  }
}  // namespace libp2p::crypto
