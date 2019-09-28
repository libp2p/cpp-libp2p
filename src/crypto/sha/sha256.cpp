/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/sha/sha256.hpp>

#include <openssl/sha.h>

namespace libp2p::crypto {
  common::Hash256 sha256(std::string_view input) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *bytes_ptr = reinterpret_cast<const uint8_t *>(input.data());
    return sha256(gsl::make_span(bytes_ptr, input.length()));
  }

  common::Hash256 sha256(gsl::span<const uint8_t> input) {
    common::Hash256 out;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, input.data(), input.size());
    SHA256_Final(out.data(), &ctx);
    return out;
  }
}  // namespace libp2p::crypto
