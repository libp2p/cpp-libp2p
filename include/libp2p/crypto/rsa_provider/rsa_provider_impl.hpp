/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <openssl/rsa.h>
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/rsa_provider.hpp>

namespace libp2p::crypto::rsa {

  /**
   * @class RSA provider implementation declaration
   */
  class RsaProviderImpl : public RsaProvider {
   public:
    outcome::result<KeyPair> generate(RSAKeyType rsa_bitness) const override;

    outcome::result<PublicKey> derive(
        const PrivateKey &private_key) const override;

    outcome::result<Signature> sign(
        BytesIn message, const PrivateKey &private_key) const override;

    outcome::result<bool> verify(BytesIn message,
                                 const Signature &signature,
                                 const PublicKey &key) const override;

   private:
    /**
     * @brief Convert key to OpenSSL type
     * @param input_key - public key bytes
     * @return Converted key or error code
     */
    static outcome::result<std::shared_ptr<X509_PUBKEY>> getPublicKeyFromBytes(
        const PublicKey &input_key);
  };
};  // namespace libp2p::crypto::rsa
