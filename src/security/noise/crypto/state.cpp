/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits>
#include <sstream>

#include <libp2p/security/noise/crypto/state.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security::noise, Error, e) {
  using E = libp2p::security::noise::Error;
  switch (e) {
    case E::INTERNAL_ERROR:
      return "Undefined behavior caused an internal error";
    case E::WRONG_KEY32_SIZE:
      return "Key size does not match to expected (32 bytes)";
    case E::EMPTY_HANDSHAKE_NAME:
      return "Handshake name cannot be empty";
    case E::WRONG_PRESHARED_KEY_SIZE:
      return "Noise spec mandates 256-bit preshared keys";
    case E::INITIALIZATION_ERROR:
      return "Handshake state is not initialized";
    case E::UNEXPECTED_WRITE_CALL:
      return "Unexpected call to WriteMessage should be ReadMessage";
    case E::NO_HANDSHAKE_MESSAGE:
      return "No handshake messages left";
    case E::LONG_MESSAGE:
      return "Message is too long";
    case E::NO_PUBLIC_KEY:
      return "Invalid state, no public key";
  }
  return "unknown error";
}

namespace libp2p::security::noise {

  outcome::result<Key32> bytesToKey32(gsl::span<const uint8_t> key) {
    Key32 result;
    if (key.size() != result.size()) {
      return Error::WRONG_KEY32_SIZE;
    }
    std::copy_n(key.begin(), result.size(), result.begin());
    return result;
  }

  // Cipher State

  CipherState::CipherState(std::shared_ptr<CipherSuite> cipher_suite, Key32 key)
      : cipher_suite_{std::move(cipher_suite)},
        key_{key},
        cipher_{cipher_suite_->cipher(key_)},
        nonce_{0} {}

  outcome::result<ByteArray> CipherState::encrypt(
      gsl::span<const uint8_t> precompiled_out,
      gsl::span<const uint8_t> plaintext, gsl::span<const uint8_t> aad) {
    auto enc_res = cipher_->encrypt(precompiled_out, nonce_, plaintext, aad);
    ++nonce_;
    return enc_res;
  }

  outcome::result<ByteArray> CipherState::decrypt(
      gsl::span<const uint8_t> precompiled_out,
      gsl::span<const uint8_t> ciphertext, gsl::span<const uint8_t> aad) {
    auto dec_res = cipher_->decrypt(precompiled_out, nonce_, ciphertext, aad);
    ++nonce_;
    return dec_res;
  }

  outcome::result<void> CipherState::rekey() {
    Key32 zeroed;
    memset(zeroed.data(), 0u, zeroed.size());
    ByteArray empty;
    OUTCOME_TRY(out_res,
                cipher_->encrypt({}, std::numeric_limits<uint64_t>::max(),
                                 zeroed, empty));
    std::copy_n(out_res.begin(), key_.size(), key_.begin());
    cipher_ = cipher_suite_->cipher(key_);
    return outcome::success();
  }

  std::shared_ptr<CipherSuite> CipherState::cipherSuite() const {
    return cipher_suite_;
  }

  // Symmetric State

  SymmetricState::SymmetricState(std::shared_ptr<CipherSuite> cipher_suite)
      : CipherState(std::move(cipher_suite), Key32{}) {}

  outcome::result<void> SymmetricState::initializeSymmetric(
      gsl::span<const uint8_t> handshake_name) {
    if (handshake_name.empty()) {
      return Error::EMPTY_HANDSHAKE_NAME;
    }
    auto hasher = cipher_suite_->hash();
    // static cast is 100% safe here
    if (static_cast<uint64_t>(handshake_name.size()) <= hasher->digestSize()) {
      hash_.resize(hasher->digestSize());
      std::copy_n(handshake_name.begin(), handshake_name.size(), hash_.begin());
    } else {
      OUTCOME_TRY(hasher->write(handshake_name));
      OUTCOME_TRY(hash_res, hasher->digest());
      hash_ = std::move(hash_res);
    }
    chaining_key_ = hash_;
    return outcome::success();
  }

  outcome::result<void> SymmetricState::mixKey(
      gsl::span<const uint8_t> dh_output) {
    nonce_ = 0;
    has_key_ = true;
    OUTCOME_TRY(
        hkdf_res,
        hkdf(cipher_suite_->hash()->hashType(), 2, chaining_key_, dh_output));
    chaining_key_ = std::move(hkdf_res.one);
    OUTCOME_TRY(hash_key, bytesToKey32(hkdf_res.two));
    key_ = hash_key;
    cipher_ = cipher_suite_->cipher(key_);
    return outcome::success();
  }

  outcome::result<void> SymmetricState::mixHash(gsl::span<const uint8_t> data) {
    auto hasher = cipher_suite_->hash();
    OUTCOME_TRY(hasher->write(hash_));
    OUTCOME_TRY(hasher->write(data));
    OUTCOME_TRY(hash_res, hasher->digest());
    hash_ = hash_res;
    return outcome::success();
  }

  outcome::result<void> SymmetricState::mixKeyAndHash(
      gsl::span<const uint8_t> data) {
    OUTCOME_TRY(
        hkdf_res,
        hkdf(cipher_suite_->hash()->hashType(), 3, chaining_key_, data));
    chaining_key_ = hkdf_res.one;  // ck
    OUTCOME_TRY(mixHash(hkdf_res.two));
    OUTCOME_TRY(hash_key, bytesToKey32(hkdf_res.three));
    key_ = hash_key;
    cipher_ = cipher_suite_->cipher(key_);
    nonce_ = 0;
    has_key_ = true;
    return outcome::success();
  }

  outcome::result<ByteArray> SymmetricState::encryptAndHash(
      gsl::span<const uint8_t> precompiled_out,
      gsl::span<const uint8_t> plaintext) {
    if (not has_key_) {
      ByteArray result(precompiled_out.size() + plaintext.size());
      OUTCOME_TRY(mixHash(plaintext));
      std::copy_n(precompiled_out.begin(), precompiled_out.size(),
                  result.begin());
      std::copy_n(plaintext.begin(), plaintext.size(),
                  result.begin() + precompiled_out.size());
      return result;
    }
    OUTCOME_TRY(ciphertext, encrypt(precompiled_out, plaintext, hash_));
    auto ct_size = ciphertext.size();
    auto po_size = precompiled_out.size();
    if (po_size > static_cast<int64_t>(ct_size)) {
      // the same gonna happen in go-libp2p, moreover it is not anyhow checked
      // and handled there!
      return Error::INTERNAL_ERROR;
    }
    ByteArray seed(ct_size - po_size);
    std::copy_n(ciphertext.begin() + po_size, seed.size(), seed.begin());
    OUTCOME_TRY(mixHash(seed));
    return std::move(ciphertext);
  }

  outcome::result<ByteArray> SymmetricState::decryptAndHash(
      gsl::span<const uint8_t> precompiled_out,
      gsl::span<const uint8_t> ciphertext) {
    if (not has_key_) {
      ByteArray result(precompiled_out.size() + ciphertext.size());
      OUTCOME_TRY(mixHash(ciphertext));
      std::copy_n(precompiled_out.begin(), precompiled_out.size(),
                  result.begin());
      std::copy_n(ciphertext.begin(), ciphertext.size(),
                  result.begin() + precompiled_out.size());
      return result;
    }
    OUTCOME_TRY(plaintext, decrypt(precompiled_out, ciphertext, hash_));
    OUTCOME_TRY(mixHash(ciphertext));
    return std::move(plaintext);
  }

  outcome::result<SymmetricState::CSPair> SymmetricState::split() {
    OUTCOME_TRY(hkdf_res,
                hkdf(cipher_suite_->hash()->hashType(), 2, chaining_key_, {}));
    OUTCOME_TRY(hash_key1, bytesToKey32(hkdf_res.one));
    OUTCOME_TRY(hash_key2, bytesToKey32(hkdf_res.two));
    auto first_state = std::make_shared<CipherState>(cipher_suite_, hash_key1);
    auto second_state = std::make_shared<CipherState>(cipher_suite_, hash_key2);
    first_state->cipher_ = cipher_suite_->cipher(first_state->key_);
    second_state->cipher_ = cipher_suite_->cipher(second_state->key_);
    return std::make_pair(std::move(first_state), std::move(second_state));
  }

  void SymmetricState::checkpoint() {
    prev_chaining_key_ = chaining_key_;
    prev_hash_ = hash_;
  }

  void SymmetricState::rollback() {
    chaining_key_ = prev_chaining_key_;
    hash_ = prev_hash_;
  }

  ByteArray SymmetricState::hash() const {
    return hash_;
  }

  outcome::result<void> HandshakeState::init(
      std::shared_ptr<CipherSuite> cipher_suite,
      const HandshakePattern &pattern, DHKey static_keypair,
      DHKey ephemeral_keypair, gsl::span<const uint8_t> remote_static_pubkey,
      gsl::span<const uint8_t> remote_ephemeral_pubkey,
      gsl::span<const uint8_t> preshared_key, int preshared_key_placement,
      MessagePatterns message_patterns, bool is_initiator,
      gsl::span<const uint8_t> prologue) {
    local_static_kp_ = std::move(static_keypair);
    local_ephemeral_kp_ = std::move(ephemeral_keypair);
    remote_static_pubkey_ = spanToVec(remote_static_pubkey);
    remote_ephemeral_pubkey_ = spanToVec(remote_ephemeral_pubkey);
    preshared_key_ = spanToVec(preshared_key);
    message_patterns_ = std::move(message_patterns);
    should_write_ = is_initiator;
    is_initiator_ = is_initiator;
    symmetric_state_ =
        std::make_unique<SymmetricState>(std::move(cipher_suite));
    std::string psk_modifier;
    if (not preshared_key_.empty()) {
      if (32 != preshared_key_.size()) {
        return Error::WRONG_PRESHARED_KEY_SIZE;
      }
      psk_modifier = "psk" + std::to_string(preshared_key_placement);
      if (0 == preshared_key_placement) {
        message_patterns_[0].insert(message_patterns_[0].begin(),
                                    MessagePattern::PSK);
      } else {
        message_patterns_[preshared_key_placement - 1].push_back(
            MessagePattern::PSK);
      }
    }
    std::stringstream ss;
    ss << "Noise_" << pattern.name << psk_modifier << "_"
       << symmetric_state_->cipherSuite()->name();
    auto handshake_name_str = ss.str();
    ByteArray handshake_name_bytes(handshake_name_str.begin(),
                                   handshake_name_str.end());
    OUTCOME_TRY(symmetric_state_->initializeSymmetric(handshake_name_bytes));
    OUTCOME_TRY(symmetric_state_->mixHash(prologue));
    for (const auto &m : pattern.initiatorPreMessages) {
      if (is_initiator_ and MessagePattern::S == m) {
        OUTCOME_TRY(symmetric_state_->mixHash(local_static_kp_.pub));
      } else if (is_initiator_ and MessagePattern::E == m) {
        OUTCOME_TRY(symmetric_state_->mixHash(local_ephemeral_kp_.pub));
      } else if (not is_initiator_ and MessagePattern::S == m) {
        OUTCOME_TRY(symmetric_state_->mixHash(remote_static_pubkey_));
      } else if (not is_initiator_ and MessagePattern::E == m) {
        OUTCOME_TRY(symmetric_state_->mixHash(remote_ephemeral_pubkey_));
      }
    }
    for (const auto &m : pattern.responderPreMessages) {
      if (not is_initiator_ and MessagePattern::S == m) {
        OUTCOME_TRY(symmetric_state_->mixHash(local_static_kp_.pub));
      } else if (not is_initiator_ and MessagePattern::E == m) {
        OUTCOME_TRY(symmetric_state_->mixHash(local_ephemeral_kp_.pub));
      } else if (is_initiator_ and MessagePattern::S == m) {
        OUTCOME_TRY(symmetric_state_->mixHash(remote_static_pubkey_));
      } else if (is_initiator_ and MessagePattern::E == m) {
        OUTCOME_TRY(symmetric_state_->mixHash(remote_ephemeral_pubkey_));
      }
    }
    is_initialized_ = true;
    return outcome::success();
  }

  outcome::result<HandshakeState::MessagingResult> HandshakeState::writeMessage(
      gsl::span<const uint8_t> precompiled_out,
      gsl::span<const uint8_t> payload) {
    OUTCOME_TRY(isInitialized());
    if (not should_write_) {
      return Error::UNEXPECTED_WRITE_CALL;
    }

    if (message_idx_ > static_cast<int64_t>(message_patterns_.size() - 1)) {
      return Error::NO_HANDSHAKE_MESSAGE;
    }

    if (static_cast<uint64_t>(payload.size()) > kMaxMsgLen) {
      return Error::LONG_MESSAGE;
    }

    auto out = spanToVec(precompiled_out);
    for (const auto &message : message_patterns_[message_idx_]) {
      switch (message) {
        case MessagePattern::E: {
          OUTCOME_TRY(ephemeral_kp,
                      symmetric_state_->cipherSuite()->generate());
          local_ephemeral_kp_ = std::move(ephemeral_kp);
          out.insert(out.end(), local_ephemeral_kp_.pub.cbegin(),
                     local_ephemeral_kp_.pub.cend());
          OUTCOME_TRY(symmetric_state_->mixHash(local_ephemeral_kp_.pub));
          if (preshared_key_.empty()) {
            OUTCOME_TRY(symmetric_state_->mixKey(local_ephemeral_kp_.pub));
          }
          break;
        }
        case MessagePattern::S: {
          if (local_static_kp_.pub.empty()) {
            return Error::NO_PUBLIC_KEY;
          }
          OUTCOME_TRY(
              output,
              symmetric_state_->encryptAndHash(out, local_static_kp_.pub));
          out = std::move(output);
          break;
        }
        case MessagePattern::DHEE: {
          OUTCOME_TRY(key,
                      symmetric_state_->cipherSuite()->dh(
                          local_ephemeral_kp_.priv, remote_ephemeral_pubkey_));
          OUTCOME_TRY(symmetric_state_->mixKey(key));
          break;
        }
        case MessagePattern::DHES: {
          if (is_initiator_) {
            OUTCOME_TRY(key,
                        symmetric_state_->cipherSuite()->dh(
                            local_ephemeral_kp_.priv, remote_static_pubkey_));
            OUTCOME_TRY(symmetric_state_->mixKey(key));
          } else {
            OUTCOME_TRY(key,
                        symmetric_state_->cipherSuite()->dh(
                            local_static_kp_.priv, remote_ephemeral_pubkey_));
            OUTCOME_TRY(symmetric_state_->mixKey(key));
          }
          break;
        }
        case MessagePattern::DHSE: {
          if (is_initiator_) {
            OUTCOME_TRY(key,
                        symmetric_state_->cipherSuite()->dh(
                            local_static_kp_.priv, remote_ephemeral_pubkey_));
            OUTCOME_TRY(symmetric_state_->mixKey(key));
          } else {
            OUTCOME_TRY(key,
                        symmetric_state_->cipherSuite()->dh(
                            local_ephemeral_kp_.priv, remote_static_pubkey_));
            OUTCOME_TRY(symmetric_state_->mixKey(key));
          }
          break;
        }
        case MessagePattern::DHSS: {
          OUTCOME_TRY(key,
                      symmetric_state_->cipherSuite()->dh(
                          local_static_kp_.priv, remote_static_pubkey_));
          OUTCOME_TRY(symmetric_state_->mixKey(key));
          break;
        }
        case MessagePattern::PSK:
          OUTCOME_TRY(symmetric_state_->mixKeyAndHash(preshared_key_));
          break;
      }
    }

    should_write_ = false;
    message_idx_++;
    OUTCOME_TRY(output, symmetric_state_->encryptAndHash(out, payload));

    if (message_idx_ >= static_cast<int64_t>(message_patterns_.size())) {
      OUTCOME_TRY(cs_pait, symmetric_state_->split());
      return HandshakeState::MessagingResult{
          .data = std::move(output),
          .cs1 = cs_pait.first,
          .cs2 = cs_pait.second,
      };
    }

    return HandshakeState::MessagingResult{
        .data = std::move(output),
        .cs1 = {},
        .cs2 = {},
    };
  }

  outcome::result<HandshakeState::MessagingResult> HandshakeState::readMessage(
      gsl::span<const uint8_t> precompiled_out,
      gsl::span<const uint8_t> message) {
    OUTCOME_TRY(isInitialized());
    // TODO
    return HandshakeState::MessagingResult{};
  }

  outcome::result<ByteArray> HandshakeState::channelBinding() const {
    OUTCOME_TRY(isInitialized());
    return symmetric_state_->hash();
  }

  outcome::result<ByteArray> HandshakeState::remotePeerStaticPubkey() const {
    OUTCOME_TRY(isInitialized());
    return remote_static_pubkey_;
  }

  outcome::result<ByteArray> HandshakeState::remotePeerEphemeralPubkey() const {
    OUTCOME_TRY(isInitialized());
    return remote_ephemeral_pubkey_;
  }

  outcome::result<DHKey> HandshakeState::localPeerEphemeralKey() const {
    OUTCOME_TRY(isInitialized());
    return local_ephemeral_kp_;
  }

  outcome::result<int> HandshakeState::messageIndex() const {
    OUTCOME_TRY(isInitialized());
    return message_idx_;
  }

  outcome::result<void> HandshakeState::isInitialized() const {
    if (is_initialized_) {
      return outcome::success();
    }
    return Error::INITIALIZATION_ERROR;
  }

}  // namespace libp2p::security::noise
