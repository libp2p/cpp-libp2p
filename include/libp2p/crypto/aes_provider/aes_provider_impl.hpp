/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP
#define LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP

#include <libp2p/crypto/aes_provider.hpp>

#include <gsl/span>
#include <libp2p/crypto/common.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto::aes {
  class AesProviderImpl : public AesProvider {
    using ByteArray = libp2p::common::ByteArray;
    using Aes128Secret = libp2p::crypto::common::Aes128Secret;
    using Aes256Secret = libp2p::crypto::common::Aes256Secret;

   public:
    outcome::result<ByteArray> encryptAesCtr128(
        const Aes128Secret &secret,
        gsl::span<const uint8_t> data) const override;

    outcome::result<ByteArray> decryptAesCtr128(
        const Aes128Secret &secret,
        gsl::span<const uint8_t> data) const override;

    outcome::result<ByteArray> encryptAesCtr256(
        const Aes256Secret &secret,
        gsl::span<const uint8_t> data) const override;

    outcome::result<ByteArray> decryptAesCtr256(
        const Aes256Secret &secret,
        gsl::span<const uint8_t> data) const override;
  };
}  // namespace libp2p::crypto::aes

#endif  // LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP
