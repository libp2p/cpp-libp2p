/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/crypto/noise_ccp1305.hpp>

namespace libp2p::security::noise {

  outcome::result<ByteArray> CCP1305Impl::encrypt(
      uint64_t nonce, gsl::span<const uint8_t> plaintext,
      gsl::span<const uint8_t> aad) {
    return std::errc::not_supported;
  }

  outcome::result<ByteArray> CCP1305Impl::decrypt(
      uint64_t nonce, gsl::span<const uint8_t> ciphertext,
      gsl::span<const uint8_t> aad) {
    return std::errc::not_supported;
  }
}  // namespace libp2p::security::noise
