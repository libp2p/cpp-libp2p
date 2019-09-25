/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_HPP

#include <outcome/outcome.hpp>
#include "common/types.hpp"
#include "crypto/common.hpp"

namespace libp2p::crypto::hmac {
  /**
   * @class HmacProvider provides HMAC functionality
   * allows calculating message authentication code
   * involving a cryptographic hash function
   * and a secret cryptographic key
   */
  class HmacProvider {
    using HashType = common::HashType;

   public:
    using ByteArray =libp2p::common::ByteArray;

    virtual ~HmacProvider() = default;
    /**
     * @brief calculates digests
     * @param hash_type hash type
     * @param key secret key
     * @param message source message
     * @return message digest if calculation was successful, error otherwise
     */
    virtual outcome::result<ByteArray> calculateDigest(
        HashType hash_type, const ByteArray &key,
        const ByteArray &message) const = 0;
  };
}  // namespace libp2p::crypto::hmac

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_HPP
