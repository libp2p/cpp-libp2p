/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_COMMON_HPP
#define LIBP2P_CRYPTO_COMMON_HPP

#include <cstdint>
#include <functional>
#include <memory>

namespace libp2p::crypto::common {
  /**
   * @struct AesSecret provides key and iv for AES cipher
   * @tparam KeySize key size
   * @tparam IvSize iv size
   */
  template <size_t key_size, size_t iv_size>
  struct AesSecret {
    std::array<uint8_t, key_size> key;
    std::array<uint8_t, iv_size> iv;
  };

  /**
   * Values for AES-128
   */
  using Aes128Secret = AesSecret<16, 16>;

  /**
   * Values for AES-256
   */
  using Aes256Secret = AesSecret<32, 16>;

  /**
   * Supported hash types
   */
  enum class HashType { SHA1, SHA256, SHA512 };

  /**
   * Supported types of RSA keys
   */
  enum class RSAKeyType { RSA1024, RSA2048, RSA4096 };

  /**
   * Supported ECDH curves
   */
  enum class CurveType { P256, P384, P521 };

  /**
   * Supported cipher types
   */
  enum class CipherType { AES128, AES256 };
}  // namespace libp2p::crypto::common

#endif  // LIBP2P_CRYPTO_COMMON_HPP
