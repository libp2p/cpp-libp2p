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
    // todo
    return HandshakeState::MessagingResult{};
  }

  outcome::result<HandshakeState::MessagingResult> HandshakeState::readMessage(
      gsl::span<const uint8_t> precompiled_out,
      gsl::span<const uint8_t> message) {
    // todo
    return HandshakeState::MessagingResult{};
  }

  ByteArray HandshakeState::channelBinding() const {
    // todo
    return libp2p::common::ByteArray();
  }

  ByteArray HandshakeState::remotePeerStaticPubkey() const {
    // todo
    return libp2p::common::ByteArray();
  }

  ByteArray HandshakeState::remotePeerEphemeralPubkey() const {
    // todo
    return libp2p::common::ByteArray();
  }

  DHKey HandshakeState::localPeerEphemeralKey() const {
    // todo
    return DHKey();
  }

  int HandshakeState::messageIndex() const {
    // todo
    return 0;
  }

}  // namespace libp2p::security::noise
