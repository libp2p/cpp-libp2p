/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/hmac_provider.hpp>
#include <span>

namespace libp2p::crypto::hmac {

  class HmacProviderImpl : public HmacProvider {
   public:
    outcome::result<Bytes> calculateDigest(HashType hash_type,
                                           const Bytes &key,
                                           BytesIn message) const override;
  };
}  // namespace libp2p::crypto::hmac
