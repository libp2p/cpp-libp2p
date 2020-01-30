/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_IMPL_HPP
#define LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_IMPL_HPP

#include <libp2p/crypto/hmac_provider.hpp>
#include <gsl/span>

namespace libp2p::crypto::hmac {
  class HmacProviderImpl : public HmacProvider {
    using HashType = common::HashType;

   public:
    outcome::result<ByteArray> calculateDigest(
        HashType hash_type, const ByteArray &key,
        gsl::span<const uint8_t> message) const override;
  };
}  // namespace libp2p::crypto::hmac

#endif  // LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_IMPL_HPP
