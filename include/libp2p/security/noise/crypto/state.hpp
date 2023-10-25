/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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

  outcome::result<Key32> bytesToKey32(BytesIn key);

  /// Provides symmetric encryption and decryption after a successful handshake
  class CipherState {
   public:
    CipherState(std::shared_ptr<CipherSuite> cipher_suite, Key32 key);

    outcome::result<Bytes> encrypt(BytesIn precompiled_out,
                                   BytesIn plaintext,
                                   BytesIn aad);

    outcome::result<Bytes> decrypt(BytesIn precompiled_out,
                                   BytesIn ciphertext,
                                   BytesIn aad);

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

    outcome::result<void> initializeSymmetric(BytesIn handshake_name);

    outcome::result<void> mixKey(BytesIn dh_output);

    outcome::result<void> mixHash(BytesIn data);

    outcome::result<void> mixKeyAndHash(BytesIn data);

    outcome::result<Bytes> encryptAndHash(BytesIn precompiled_out,
                                          BytesIn plaintext);

    outcome::result<Bytes> decryptAndHash(BytesIn precompiled_out,
                                          BytesIn ciphertext);

    outcome::result<CSPair> split();

    void checkpoint();

    void rollback();

    Bytes hash() const;

    bool hasKey() const;

   private:
    bool has_key_ = false;     // has_k
    Bytes chaining_key_;       // ck
    Bytes hash_;               // h
    Bytes prev_chaining_key_;  // prev_ck
    Bytes prev_hash_;          // prev_h
  };

  constexpr size_t kMaxMsgLen = 65535;
  constexpr size_t kTagSize = 16;
  constexpr size_t kMaxPlainText = kMaxMsgLen - kTagSize;
  constexpr size_t kLengthPrefixSize = 2;  // in bytes

  class HandshakeStateConfig {
   public:
    HandshakeStateConfig(std::shared_ptr<CipherSuite> cipher_suite,
                         HandshakePattern pattern,
                         bool is_initiator,
                         DHKey local_static_keypair);

    HandshakeStateConfig &setPrologue(BytesIn prologue);

    HandshakeStateConfig &setPresharedKey(BytesIn key, int placement);

    HandshakeStateConfig &setLocalEphemeralKeypair(DHKey keypair);

    HandshakeStateConfig &setRemoteStaticPubkey(BytesIn key);

    HandshakeStateConfig &setRemoteEphemeralPubkey(BytesIn key);

   private:
    template <typename T>
    using opt = boost::optional<T>;
    friend class HandshakeState;

    std::shared_ptr<CipherSuite> cipher_suite_;
    HandshakePattern pattern_;
    bool is_initiator_;
    DHKey local_static_keypair_;
    // optional fields go below
    opt<Bytes> prologue_;
    opt<Bytes> preshared_key_;
    opt<int> preshared_key_placement_;
    opt<DHKey> local_ephemeral_keypair_;
    opt<Bytes> remote_static_pubkey_;
    opt<Bytes> remote_ephemeral_pubkey_;
  };

  class HandshakeState {
   public:
    struct MessagingResult {
      Bytes data;
      std::shared_ptr<CipherState> cs1;
      std::shared_ptr<CipherState> cs2;
    };

    HandshakeState() = default;

    outcome::result<void> init(HandshakeStateConfig config);

    outcome::result<MessagingResult> writeMessage(BytesIn precompiled_out,
                                                  BytesIn payload);

    outcome::result<MessagingResult> readMessage(BytesIn precompiled_out,
                                                 BytesIn message);

    outcome::result<Bytes> channelBinding() const;

    outcome::result<Bytes> remotePeerStaticPubkey() const;

    outcome::result<Bytes> remotePeerEphemeralPubkey() const;

    outcome::result<DHKey> localPeerEphemeralKey() const;

    outcome::result<int> messageIndex() const;

   private:
    outcome::result<void> isInitialized() const;

    outcome::result<void> writeMessageE(Bytes &out);

    outcome::result<void> writeMessageS(Bytes &out);

    outcome::result<void> writeMessageDHEE();

    outcome::result<void> writeMessageDHES();

    outcome::result<void> writeMessageDHSE();

    outcome::result<void> writeMessageDHSS();

    outcome::result<void> writeMessagePSK();

    outcome::result<void> readMessageE(Bytes &message);

    outcome::result<void> readMessageS(Bytes &message);

    outcome::result<void> readMessageDHEE();

    outcome::result<void> readMessageDHES();

    outcome::result<void> readMessageDHSE();

    outcome::result<void> readMessageDHSS();

    outcome::result<void> readMessagePSK();

    bool is_initialized_ = false;
    std::unique_ptr<SymmetricState> symmetric_state_;  // ss
    DHKey local_static_kp_;                            // s
    DHKey local_ephemeral_kp_;                         // e
    Bytes remote_static_pubkey_;                       // rs
    Bytes remote_ephemeral_pubkey_;                    // re
    Bytes preshared_key_;                              // psk
    MessagePatterns message_patterns_;
    bool should_write_;
    bool is_initiator_;
    int message_idx_;
  };

}  // namespace libp2p::security::noise

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::noise, Error);
