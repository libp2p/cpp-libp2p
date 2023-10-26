/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>

#include <libp2p/common/types.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::ed25519 {

  using PrivateKey = std::array<uint8_t, 32u>;
  using PublicKey = std::array<uint8_t, 32u>;
  struct Keypair {
    PrivateKey private_key;
    PublicKey public_key;
  };
  using Signature = std::array<uint8_t, 64u>;

  /**
   * An interface for Ed25519 private/public key cryptography operations.
   */
  class Ed25519Provider {
   public:
    /**
     * Generate keypair
     * @return a struct with private and public keys as byte arrays
     */
    virtual outcome::result<Keypair> generate() const = 0;

    /**
     * Derive public key from a given private key
     * @param private_key - bytes representation of a private key
     * @return public key as byte array
     */
    virtual outcome::result<PublicKey> derive(
        const PrivateKey &private_key) const = 0;

    /**
     * Sign a message using private key. Message digest calculated as SHA512
     * @param message - the source message as bytes sequence
     * @param private_key - private key bytes
     * @return signature as bytes sequence
     */
    virtual outcome::result<Signature> sign(
        BytesIn message, const PrivateKey &private_key) const = 0;

    /**
     * Verify signature of a message against a given public key
     * @param message - the source message as bytes sequence
     * @param signature - signature bytes
     * @param public_key - public key bytes
     * @return - true when signature is valid, false - otherwise
     */
    virtual outcome::result<bool> verify(BytesIn message,
                                         const Signature &signature,
                                         const PublicKey &public_key) const = 0;

    virtual ~Ed25519Provider() = default;
  };

}  // namespace libp2p::crypto::ed25519
