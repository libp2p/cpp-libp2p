/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <openssl/evp.h>
#include <libp2p/crypto/hmac_provider.hpp>
#include <span>

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
