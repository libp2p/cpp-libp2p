/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <openssl/ec.h>
#include <libp2p/crypto/secp256k1_provider.hpp>

namespace libp2p::crypto::secp256k1 {
  class Secp256k1ProviderImpl : public Secp256k1Provider {
   public:
    outcome::result<KeyPair> generate() const override;

    outcome::result<PublicKey> derive(const PrivateKey &key) const override;

    outcome::result<Signature> sign(BytesIn message,
                                    const PrivateKey &key) const override;

    outcome::result<bool> verify(BytesIn message,
                                 const Signature &signature,
                                 const PublicKey &key) const override;

   private:
    /**
     * @brief Convert secp256k1 private key bytes to OpenSSL EC_KEY structure
     * @param input - private key bytes
     * @return OpenSSL EC_KEY structure or error code
     */
    static outcome::result<std::shared_ptr<EC_KEY>> bytesToPrivateKey(
        const PrivateKey &input);

    /**
     * @brief Convert secp256k1 public key bytes to OpenSSL EC_KEY structure
     * @param input - private key bytes
     * @return OpenSSL EC_KEY structure or error code
     */
    static outcome::result<std::shared_ptr<EC_KEY>> bytesToPublicKey(
        const PublicKey &input);
  };
}  // namespace libp2p::crypto::secp256k1
