/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_CTR_IMPL_HPP
#define LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_CTR_IMPL_HPP

#include <openssl/evp.h>
#include <span>
#include <libp2p/crypto/hmac_provider.hpp>

namespace libp2p::crypto::hmac {

  class HmacProviderCtrImpl : public HmacProviderCtr {
   public:
    HmacProviderCtrImpl(HashType hash_type, BytesIn key);

    ~HmacProviderCtrImpl() override;

    outcome::result<void> write(BytesIn data) override;

    outcome::result<void> digestOut(BytesOut out) const override;

    outcome::result<void> reset() override;

    size_t digestSize() const override;

    size_t blockSize() const override;

    HashType hashType() const override;

   private:
    void sinkCtx(size_t digest_size);

    HashType hash_type_;
    Bytes key_;
    const EVP_MD *hash_st_;
    HMAC_CTX *hmac_ctx_;
    bool initialized_;
  };

}  // namespace libp2p::crypto::hmac

#endif  // LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_CTR_IMPL_HPP
