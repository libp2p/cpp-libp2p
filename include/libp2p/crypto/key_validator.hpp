/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/key.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::validator {

  class KeyValidator {
   public:
    virtual ~KeyValidator() = default;

    /**
     * @brief validates private key
     * @param key const reference to private key
     * @return success if key is valid and error otherwise
     */
    virtual outcome::result<void> validate(const PrivateKey &key) const = 0;

    /**
     * @brief validates public key
     * @param key const reference to public key
     * @return success if key is valid and error otherwise
     */
    virtual outcome::result<void> validate(const PublicKey &key) const = 0;

    /**
     * @brief validates pair of keys
     * @param keys const reference to pair of keys
     * @return success if key pair is valid and error otherwise
     */
    virtual outcome::result<void> validate(const KeyPair &keys) const = 0;
  };

}  // namespace libp2p::crypto::validator
