/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits>

#include <libp2p/security/noise/crypto/state.hpp>

namespace libp2p::security::noise {

  // Cipher State

  CipherState::CipherState(std::shared_ptr<CipherSuite> cipher_suite, Key32 key)
      : cipher_suite_{std::move(cipher_suite)},
        key_{key},
        cipher_{cipher_suite_->cipher(key_)},
        nonce_{0} {}

  outcome::result<ByteArray> CipherState::encrypt(
      gsl::span<const uint8_t> plaintext, gsl::span<const uint8_t> aad) {
    auto enc_res = cipher_->encrypt(nonce_, plaintext, aad);
    ++nonce_;
    return enc_res;
  }

  outcome::result<ByteArray> CipherState::decrypt(
      gsl::span<const uint8_t> ciphertext, gsl::span<const uint8_t> aad) {
    auto dec_res = cipher_->decrypt(nonce_, ciphertext, aad);
    ++nonce_;
    return dec_res;
  }

  outcome::result<void> CipherState::rekey() {
    Key32 zeroed;
    memset(zeroed.data(), 0u, zeroed.size());
    ByteArray empty;
    OUTCOME_TRY(
        out_res,
        cipher_->encrypt(std::numeric_limits<uint64_t>::max(), zeroed, empty));
    std::copy_n(out_res.begin(), key_.size(), key_.begin());
    cipher_ = cipher_suite_->cipher(key_);
    return outcome::success();
  }

  //  std::shared_ptr<CipherSuite> CipherState::cipherSuite() {
  //    return cipher_suite_;
  //  }

  // Symmetric State

  outcome::result<void> SymmetricState::initializeSymmetric(
      gsl::span<const uint8_t> handshake_name) {
    BOOST_ASSERT(handshake_name.size() > 0);
    auto hasher = cipher_suite_->hash();
    // static cast is 100% safe here
    if (static_cast<uint64_t>(handshake_name.size()) <= hasher->digestSize()) {
      h_.resize(hasher->digestSize());
      std::copy_n(handshake_name.begin(), handshake_name.size(), h_.begin());
    } else {
      OUTCOME_TRY(hasher->write(handshake_name));
      OUTCOME_TRY(hash_res, hasher->digest());
      h_ = std::move(hash_res);
    }
    chaining_key_ = h_;
    return outcome::success();
  }

  outcome::result<void> SymmetricState::mixKey(
      gsl::span<const uint8_t> dh_output) {
    nonce_ = 0;
    has_key_ = true;
    OUTCOME_TRY(
        hkdf_res,
        hkdf(cipher_suite_->hash()->hashType(), 2, chaining_key_, dh_output));
    chaining_key_ = hkdf_res.one;
    BOOST_ASSERT(hkdf_res.two.size() == key_.size());
    key_ = asArray<Key32>(hkdf_res.two);  // hk in golang impl
    cipher_ = cipher_suite_->cipher(key_);
    return outcome::success();
  }

  outcome::result<void> SymmetricState::mixHash(gsl::span<const uint8_t> data) {
    auto hasher = cipher_suite_->hash();
    OUTCOME_TRY(hasher->write(h_));
    OUTCOME_TRY(hasher->write(data));
    OUTCOME_TRY(hash_res, hasher->digest());
    h_ = hash_res;
    return outcome::success();
  }

  outcome::result<void> SymmetricState::mixKeyAndHash(
      gsl::span<const uint8_t> data) {
    OUTCOME_TRY(
        hkdf_res,
        hkdf(cipher_suite_->hash()->hashType(), 3, chaining_key_, data));
    chaining_key_ = hkdf_res.one;  // ck
    OUTCOME_TRY(mixHash(hkdf_res.two));
    BOOST_ASSERT(hkdf_res.three.size() == key_.size());
    key_ = asArray<Key32>(hkdf_res.three);
    cipher_ = cipher_suite_->cipher(key_);
    nonce_ = 0;
    has_key_ = true;
    return outcome::success();
  }

//  outcome::result<ByteArray> SymmetricState::encryptAndHash(gsl::span<const uint8_t> plaintext) {
//      if (not has_key_) {
//          OUTCOME_TRY(mixHash(plaintext));
//          return span2vec(plaintext);
//      }
//      OUTCOME_TRY(ciphertext, encrypt(plaintext, h_));
//      OUTCOME_TRY(mixHash())
//  }

  outcome::result<SymmetricState::CSPair> SymmetricState::split() {
    OUTCOME_TRY(hkdf_res,
                hkdf(cipher_suite_->hash()->hashType(), 2, chaining_key_, {}));
    BOOST_ASSERT(hkdf_res.one.size() == key_.size());
    std::shared_ptr<CipherState> first_state = std::make_shared<CipherState>(
        cipher_suite_, asArray<Key32>(hkdf_res.one));
    BOOST_ASSERT(hkdf_res.two.size() == key_.size());
    std::shared_ptr<CipherState> second_state = std::make_shared<CipherState>(
        cipher_suite_, asArray<Key32>(hkdf_res.two));
    first_state->cipher_ = cipher_suite_->cipher(first_state->key_);
    second_state->cipher_ = cipher_suite_->cipher(second_state->key_);
    return std::make_pair(std::move(first_state), std::move(second_state));
  }

  void SymmetricState::checkpoint() {
    if (chaining_key_.size() > prev_ck_.capacity()
        || prev_ck_.size() > chaining_key_.size()) {
      prev_ck_.resize(chaining_key_.size());
    }
    std::copy(chaining_key_.begin(), chaining_key_.end(), prev_ck_.begin());

    if (h_.size() > prev_h_.capacity() || prev_h_.size() > h_.size()) {
      prev_h_.resize(h_.size());
    }
    std::copy(h_.begin(), h_.end(), prev_h_.begin());
  }

  void SymmetricState::rollback() {
    if (prev_ck_.size() > chaining_key_.capacity()
        || chaining_key_.size() > prev_ck_.size()) {
      chaining_key_.resize(prev_ck_.size());
    }
    std::copy(prev_ck_.begin(), prev_ck_.end(), chaining_key_.begin());

    if (prev_h_.size() > h_.capacity() || h_.size() > prev_h_.size()) {
      h_.resize(prev_h_.size());
    }
    std::copy(prev_h_.begin(), prev_h_.end(), h_.begin());
  }

}  // namespace libp2p::security::noise
