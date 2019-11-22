/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/sha/sha512.hpp>

#include <openssl/sha.h>

namespace libp2p::crypto {
  common::Hash512 sha512(std::string_view input) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *bytes_ptr = reinterpret_cast<const uint8_t *>(input.data());
    return sha512(gsl::make_span(bytes_ptr, input.length()));
  }

  common::Hash512 sha512(gsl::span<const uint8_t> input) {
    common::Hash512 out;
    SHA512_CTX ctx;
    SHA512_Init(&ctx);
    SHA512_Update(&ctx, input.data(), input.size());
    SHA512_Final(out.data(), &ctx);
    return out;
  }
}  // namespace libp2p::crypto
