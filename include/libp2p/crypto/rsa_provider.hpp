/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_RSA_PROVIDER_HPP
#define LIBP2P_CRYPTO_RSA_PROVIDER_HPP

#include <gsl/span>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/crypto/rsa_types.hpp>

namespace libp2p::crypto::rsa {

  /**
   * @class RSA provider interface
   */
  class RsaProvider {
   public:
    /**
     * Sign a message using private key. Message digest calculated as SHA512
     * @param message - the source message as bytes sequence
     * @param private_key - private key bytes
     * @return signature as bytes sequence
     */
    virtual outcome::result<Signature> sign(gsl::span<uint8_t> message,
                                            const PrivateKey &private_key) const = 0;

    /**
     * @brief Verify signature for a message
     * @param message - signed data
     * @param signature - target for verifying
     * @param key - key for signature verifying
     * @return Result of the verification or error code
     */
    virtual outcome::result<bool> verify(gsl::span<uint8_t> message,
                                         const Signature &signature,
                                         const PublicKey &key) const = 0;

    virtual ~RsaProvider() = default;
  };
};  // namespace libp2p::crypto::rsa

#endif  // LIBP2P_CRYPTO_RSA_PROVIDER_HPP
