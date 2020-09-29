/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_STATE_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_STATE_HPP

#include <libp2p/security/noise/crypto/interfaces.hpp>

namespace libp2p::security::noise {

  enum class Error {
    INTERNAL_ERROR = 1,
    WRONG_KEY32_SIZE,
    EMPTY_HANDSHAKE_NAME,
    WRONG_PRESHARED_KEY_SIZE,
  };

  outcome::result<Key32> bytesToKey32(gsl::span<const uint8_t> key);

  /// Provides symmetric encryption and decryption after a successful handshake
  class CipherState {
   public:
    CipherState(std::shared_ptr<CipherSuite> cipher_suite, Key32 key);

    outcome::result<ByteArray> encrypt(gsl::span<const uint8_t> precompiled_out,
                                       gsl::span<const uint8_t> plaintext,
                                       gsl::span<const uint8_t> aad);

    outcome::result<ByteArray> decrypt(gsl::span<const uint8_t> precompiled_out,
                                       gsl::span<const uint8_t> ciphertext,
                                       gsl::span<const uint8_t> aad);

    outcome::result<void> rekey();

    std::shared_ptr<CipherSuite> cipherSuite() const;

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

    explicit SymmetricState(std::shared_ptr<CipherSuite> cipher_suite);

    //    using CipherState::CipherState; ??

    outcome::result<void> initializeSymmetric(
        gsl::span<const uint8_t> handshake_name);

    outcome::result<void> mixKey(gsl::span<const uint8_t> dh_output);

    outcome::result<void> mixHash(gsl::span<const uint8_t> data);

    outcome::result<void> mixKeyAndHash(gsl::span<const uint8_t> data);

    outcome::result<ByteArray> encryptAndHash(
        gsl::span<const uint8_t> precompiled_out,
        gsl::span<const uint8_t> plaintext);

    outcome::result<ByteArray> decryptAndHash(
        gsl::span<const uint8_t> precompiled_out,
        gsl::span<const uint8_t> ciphertext);

    outcome::result<CSPair> split();

    void checkpoint();

    void rollback();

   private:
    // names below has come from go-libp2p-noise :(
    bool has_key_ = false;         // has_k
    ByteArray chaining_key_;       // ck
    ByteArray hash_;               // h
    ByteArray prev_chaining_key_;  // prev_ck
    ByteArray prev_hash_;          // prev_h
  };

  enum class MessagePattern : int {  // int required here
    S,
    E,
    DHEE,
    DHES,
    DHSE,
    DHSS,
    PSK  // no elements anymore
  };

  using MessagePatterns = std::vector<std::vector<MessagePattern>>;

  struct HandshakePattern {
    std::string name;
    std::vector<MessagePattern> initiatorPreMessages;
    std::vector<MessagePattern> responderPreMessages;
    MessagePatterns messages;
  };

  constexpr size_t kMaxMsgLen = 65535;

  class HandshakeState {
   public:
    struct MessagingResult {
      ByteArray data;
      std::shared_ptr<CipherState> cs1;
      std::shared_ptr<CipherState> cs2;
    };

    HandshakeState() = default;

    outcome::result<void> init(std::shared_ptr<CipherSuite> cipher_suite,
                               const HandshakePattern &pattern,
                               DHKey static_keypair, DHKey ephemeral_keypair,
                               gsl::span<const uint8_t> remote_static_pubkey,
                               gsl::span<const uint8_t> remote_ephemeral_pubkey,
                               gsl::span<const uint8_t> preshared_key,
                               int preshared_key_placement,
                               MessagePatterns message_patterns,
                               bool is_initiator,
                               gsl::span<const uint8_t> prologue);

    outcome::result<MessagingResult> writeMessage(
        gsl::span<const uint8_t> precompiled_out,
        gsl::span<const uint8_t> payload);

    outcome::result<MessagingResult> readMessage(
        gsl::span<const uint8_t> precompiled_out,
        gsl::span<const uint8_t> message);

    ByteArray channelBinding() const;

    ByteArray remotePeerStaticPubkey() const;

    ByteArray remotePeerEphemeralPubkey() const;

    DHKey localPeerEphemeralKey() const;

    int messageIndex() const;

   private:
    bool is_initialized_ = false;
    std::unique_ptr<SymmetricState> symmetric_state_;  // ss
    DHKey local_static_kp_;                            // s
    DHKey local_ephemeral_kp_;                         // e
    ByteArray remote_static_pubkey_;                   // rs
    ByteArray remote_ephemeral_pubkey_;                // re
    ByteArray preshared_key_;                          // psk
    MessagePatterns message_patterns_;
    bool should_write_;
    bool is_initiator_;
    int message_idx_;
  };

}  // namespace libp2p::security::noise

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::noise, Error);

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_STATE_HPP