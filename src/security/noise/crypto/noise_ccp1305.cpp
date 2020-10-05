/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/crypto/noise_ccp1305.hpp>

namespace libp2p::security::noise {

  namespace {
    template <typename T>
    void unused(T &&) {}
  }  // namespace

  CCP1305Impl::CCP1305Impl(Key32 key)
      : ccp_{std::make_unique<crypto::chachapoly::ChaCha20Poly1305Impl>(key)} {}

  outcome::result<ByteArray> CCP1305Impl::encrypt(
      gsl::span<const uint8_t> precompiled_out, uint64_t nonce,
      gsl::span<const uint8_t> plaintext, gsl::span<const uint8_t> aad) {
    auto n = ccp_->uint64toNonce(nonce);
    OUTCOME_TRY(enc, ccp_->encrypt(n, plaintext, aad));
    auto res = spanToVec(precompiled_out);
    res.reserve(res.size() + enc.size());
    res.insert(res.end(), enc.begin(), enc.end());
    return res;
  }

  outcome::result<ByteArray> CCP1305Impl::decrypt(
      gsl::span<const uint8_t> precompiled_out, uint64_t nonce,
      gsl::span<const uint8_t> ciphertext, gsl::span<const uint8_t> aad) {
    auto n = ccp_->uint64toNonce(nonce);
    OUTCOME_TRY(dec, ccp_->decrypt(n, ciphertext, aad));
    auto res = spanToVec(precompiled_out);
    res.reserve(res.size() + dec.size());
    res.insert(res.end(), dec.begin(), dec.end());
    return res;
  }

  std::shared_ptr<AEADCipher> NamedCCPImpl::cipher(Key32 key) {
    return std::make_shared<CCP1305Impl>(key);
  }

  std::string NamedCCPImpl::cipherName() const {
    return "ChaChaPoly";
  }
}  // namespace libp2p::security::noise
