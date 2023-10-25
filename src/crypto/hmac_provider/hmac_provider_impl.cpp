/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/hmac_provider/hmac_provider_ctr_impl.hpp>
#include <libp2p/crypto/hmac_provider/hmac_provider_impl.hpp>

#include <span>

namespace libp2p::crypto::hmac {
  using ByteArray = libp2p::Bytes;

  outcome::result<ByteArray> HmacProviderImpl::calculateDigest(
      HashType hash_type, const ByteArray &key, BytesIn message) const {
    HmacProviderCtrImpl hmac{hash_type, key};
    OUTCOME_TRY(hmac.write(message));
    return hmac.digest();
  }

}  // namespace libp2p::crypto::hmac
