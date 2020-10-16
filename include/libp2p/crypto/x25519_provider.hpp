/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_CRYPTO_X25519_PROVIDER_HPP
#define LIBP2P_INCLUDE_LIBP2P_CRYPTO_X25519_PROVIDER_HPP

#include <array>
#include <vector>

#include <gsl/span>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::x25519 {

  using PrivateKey = std::array<uint8_t, 32u>;
  using PublicKey = std::array<uint8_t, 32u>;
  struct Keypair {
    PrivateKey private_key;
    PublicKey public_key;
  };

  /**
   * Diffie Hellman key agreement calculator
   */
  class X25519Provider {
   public:
    virtual ~X25519Provider() = default;

    /**
     * Generates a keypair using x25519 algo
     * @return a keypair
     */
    virtual outcome::result<Keypair> generate() const = 0;

    /**
     * Produces public key counterpart for the given private key bytes
     * @param private_key bytes
     * @return public key bytes
     */
    virtual outcome::result<PublicKey> derive(
        const PrivateKey &private_key) const = 0;

    /**
     * Does dh calculation - derives shared secret
     * @param private_key
     * @param public_key
     * @return dh result as bytes
     */
    virtual outcome::result<std::vector<uint8_t>> dh(
        const PrivateKey &private_key, const PublicKey &public_key) const = 0;
  };

}  // namespace libp2p::crypto::x25519

#endif  // LIBP2P_INCLUDE_LIBP2P_CRYPTO_X25519_PROVIDER_HPP
