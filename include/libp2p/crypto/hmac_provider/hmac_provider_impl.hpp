/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_IMPL_HPP
#define LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_IMPL_HPP

#include <openssl/evp.h>
#include <gsl/span>
#include <libp2p/crypto/hmac_provider.hpp>

namespace libp2p::crypto::hmac {
  class HmacProviderImpl : public HmacProvider {
    using HashType = common::HashType;

   public:
    [[deprecated]] HmacProviderImpl();

    HmacProviderImpl(HashType hash_type, const ByteArray &key);

    ~HmacProviderImpl() override;

    outcome::result<void> write(gsl::span<const uint8_t> data) override;

    outcome::result<std::vector<uint8_t>> digest() override;

    outcome::result<void> reset() override;

    size_t digestSize() const override;

    size_t blockSize() const override;

    [[deprecated]] outcome::result<ByteArray> calculateDigest(
        HashType hash_type, const ByteArray &key,
        gsl::span<const uint8_t> message) const override;

   private:
    void sinkCtx();

    HashType hash_type_;
    ByteArray key_;
    const EVP_MD *hash_st_;
    HMAC_CTX *hmac_ctx_;
    bool initialized_;
  };
}  // namespace libp2p::crypto::hmac

#endif  // LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_IMPL_HPP
