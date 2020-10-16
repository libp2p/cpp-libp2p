/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_HPP
#define LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_HPP

#include <gsl/span>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/hasher.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::hmac {

  using ByteArray = libp2p::common::ByteArray;
  using HashType = common::HashType;

  /// HMAC that supports stream data feeding interface
  class HmacProviderCtr : public Hasher {};

  /**
   * @class HmacProvider provides HMAC functionality
   * allows calculating message authentication code
   * involving a cryptographic hash function
   * and a secret cryptographic key
   */
  class HmacProvider {
   public:
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
        gsl::span<const uint8_t> message) const = 0;
  };
}  // namespace libp2p::crypto::hmac

#endif  // LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_HPP
