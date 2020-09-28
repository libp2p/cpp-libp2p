/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_STATE_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_STATE_HPP

#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {

  /// Provides symmetric encryption and decryption after a successful handshake
  class CipherState {
   public:
    CipherState(std::shared_ptr<CipherSuite> cipher_suite, Key32 key);

    outcome::result<ByteArray> encrypt(gsl::span<const uint8_t> plaintext,
                                       gsl::span<const uint8_t> aad);

    outcome::result<ByteArray> decrypt(gsl::span<const uint8_t> ciphertext,
                                       gsl::span<const uint8_t> aad);

    outcome::result<void> rekey();

   private:
    friend class SymmetricState;

    std::shared_ptr<CipherSuite> cipher_suite_;  // cs
    Key32 key_;                                  // k
    std::shared_ptr<AEADCipher> cipher_;         // c
    uint64_t nonce_;                             // n
  };

  class SymmetricState : public CipherState {
   public:
    using CSPair =
        std::pair<std::shared_ptr<CipherState>, std::shared_ptr<CipherState>>;

    outcome::result<void> initializeSymmetric(
        gsl::span<const uint8_t> handshake_name);

    outcome::result<void> mixKey(gsl::span<const uint8_t> dh_output);

    outcome::result<void> mixHash(gsl::span<const uint8_t> data);

    outcome::result<void> mixKeyAndHash(gsl::span<const uint8_t> data);

    outcome::result<ByteArray> encryptAndHash(
        gsl::span<const uint8_t> plaintext);

    outcome::result<CSPair> split();

    void checkpoint();

    void rollback();

   private:
    // names below has come from go-libp2p-noise :(
    bool has_key_ = false;    // has_k
    ByteArray chaining_key_;  // ck
    ByteArray h_;             // h
    ByteArray prev_ck_;       // prev_ck
    ByteArray prev_h_;        // prev_h
  };
}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_STATE_HPP
