/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_STATE_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_STATE_HPP

#include <boost/optional.hpp>

#include <libp2p/security/noise/crypto/interfaces.hpp>
#include <libp2p/security/noise/crypto/message_patterns.hpp>

namespace libp2p::security::noise {

  enum class Error {
    INTERNAL_ERROR = 1,
    WRONG_KEY32_SIZE,
    EMPTY_HANDSHAKE_NAME,
    WRONG_PRESHARED_KEY_SIZE,
    NOT_INITIALIZED,
    UNEXPECTED_WRITE_CALL,
    UNEXPECTED_READ_CALL,
    NO_HANDSHAKE_MESSAGE,
    MESSAGE_TOO_LONG,
    MESSAGE_TOO_SHORT,
    NO_PUBLIC_KEY,
    REMOTE_KEY_ALREADY_SET,
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

    ByteArray hash() const;

    bool hasKey() const;

   private:
    bool has_key_ = false;         // has_k
    ByteArray chaining_key_;       // ck
    ByteArray hash_;               // h
    ByteArray prev_chaining_key_;  // prev_ck
    ByteArray prev_hash_;          // prev_h
  };

  constexpr size_t kMaxMsgLen = 65535;
  constexpr size_t kTagSize = 16;
  constexpr size_t kMaxPlainText = kMaxMsgLen - kTagSize;
  constexpr size_t kLengthPrefixSize = 2;  // in bytes

  class HandshakeStateConfig {
   public:
    HandshakeStateConfig(std::shared_ptr<CipherSuite> cipher_suite,
                         HandshakePattern pattern, bool is_initiator,
                         DHKey local_static_keypair);

    HandshakeStateConfig &setPrologue(gsl::span<const uint8_t> prologue);

    HandshakeStateConfig &setPresharedKey(gsl::span<const uint8_t> key,
                                          int placement);

    HandshakeStateConfig &setLocalEphemeralKeypair(DHKey keypair);

    HandshakeStateConfig &setRemoteStaticPubkey(gsl::span<const uint8_t> key);

    HandshakeStateConfig &setRemoteEphemeralPubkey(
        gsl::span<const uint8_t> key);

   private:
    template <typename T>
    using opt = boost::optional<T>;
    friend class HandshakeState;

    std::shared_ptr<CipherSuite> cipher_suite_;
    HandshakePattern pattern_;
    bool is_initiator_;
    DHKey local_static_keypair_;
    // optional fields go below
    opt<ByteArray> prologue_;
    opt<ByteArray> preshared_key_;
    opt<int> preshared_key_placement_;
    opt<DHKey> local_ephemeral_keypair_;
    opt<ByteArray> remote_static_pubkey_;
    opt<ByteArray> remote_ephemeral_pubkey_;
  };

  class HandshakeState {
   public:
    struct MessagingResult {
      ByteArray data;
      std::shared_ptr<CipherState> cs1;
      std::shared_ptr<CipherState> cs2;
    };

    HandshakeState() = default;

    outcome::result<void> init(HandshakeStateConfig config);

    outcome::result<MessagingResult> writeMessage(
        gsl::span<const uint8_t> precompiled_out,
        gsl::span<const uint8_t> payload);

    outcome::result<MessagingResult> readMessage(
        gsl::span<const uint8_t> precompiled_out,
        gsl::span<const uint8_t> message);

    outcome::result<ByteArray> channelBinding() const;

    outcome::result<ByteArray> remotePeerStaticPubkey() const;

    outcome::result<ByteArray> remotePeerEphemeralPubkey() const;

    outcome::result<DHKey> localPeerEphemeralKey() const;

    outcome::result<int> messageIndex() const;

   private:
    outcome::result<void> isInitialized() const;

    outcome::result<void> writeMessageE(ByteArray &out);

    outcome::result<void> writeMessageS(ByteArray &out);

    outcome::result<void> writeMessageDHEE();

    outcome::result<void> writeMessageDHES();

    outcome::result<void> writeMessageDHSE();

    outcome::result<void> writeMessageDHSS();

    outcome::result<void> writeMessagePSK();

    outcome::result<void> readMessageE(ByteArray &message);

    outcome::result<void> readMessageS(ByteArray &message);

    outcome::result<void> readMessageDHEE();

    outcome::result<void> readMessageDHES();

    outcome::result<void> readMessageDHSE();

    outcome::result<void> readMessageDHSS();

    outcome::result<void> readMessagePSK();

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
