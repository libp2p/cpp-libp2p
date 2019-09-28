/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/key_validator/key_validator_impl.hpp>

#include <openssl/rsa.h>
#include <gsl/gsl_util>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto::validator {

  namespace {
    constexpr int getRsaBitsCount(Key::Type key_type) {
      switch (key_type) {
        case Key::Type::RSA1024:
          return 1024;
        case Key::Type::RSA2048:
          return 2048;
        case Key::Type::RSA4096:
          return 4096;
        default:
          return 0;
      }
    }

    // ed25519 private key has fixed size
    constexpr size_t ED25519_PRIVATE_KEY_SIZE = 32u;
    // ed25519 public key has fixed size
    constexpr size_t ED25519_PUBLIC_KEY_SIZE = 32u;
    // secp256k1 private key has fixed size
    constexpr size_t SECP256K1_PRIVATE_KEY_SIZE = 32u;
    // secp256k1 public key has fixed size
    constexpr size_t SECP256K1_PUBLIC_KEY_SIZE = 33u;
    // secp256k1 public key is a 32-byte sequence prepended by a prefix
    // 0x02 for even
    constexpr uint8_t SECP256K1_PUBLIC_KEY_EVEN_PREFIX = 0x02;
    // 0x03 for odd
    constexpr uint8_t SECP256K1_PUBLIC_KEY_ODD_PREFIX = 0x03;
  }  // namespace

  KeyValidatorImpl::KeyValidatorImpl(
      std::shared_ptr<KeyGenerator> key_generator)
      : key_generator_{std::move(key_generator)} {}

  outcome::result<void> KeyValidatorImpl::validate(
      const PrivateKey &key) const {
    switch (key.type) {
      case Key::Type::RSA1024:
      case Key::Type::RSA2048:
      case Key::Type::RSA4096:
        return validateRsa(key);
      case Key::Type::ED25519:
        return validateEd25519(key);
      case Key::Type::SECP256K1:
        return validateSecp256k1(key);
      case Key::Type::UNSPECIFIED:  // consider unspecified key valid
        break;
    }

    return outcome::success();
  }

  outcome::result<void> KeyValidatorImpl::validate(const PublicKey &key) const {
    switch (key.type) {
      case Key::Type::RSA1024:
      case Key::Type::RSA2048:
      case Key::Type::RSA4096:
        return validateRsa(key);
      case Key::Type::ED25519:
        return validateEd25519(key);
      case Key::Type::SECP256K1:
        return validateSecp256k1(key);
      case Key::Type::UNSPECIFIED:  // consider unspecified key valid
        break;
    }

    return outcome::success();
  }

  outcome::result<void> KeyValidatorImpl::validate(const KeyPair &keys) const {
    if (keys.privateKey.type != keys.publicKey.type) {
      return KeyValidatorError::DIFFERENT_KEY_TYPES;
    }

    if (keys.privateKey.type == Key::Type::UNSPECIFIED) {
      return outcome::success();  // consider unspecified key valid
    }

    OUTCOME_TRY(validate(keys.privateKey));
    OUTCOME_TRY(validate(keys.publicKey));

    OUTCOME_TRY(public_key, key_generator_->derivePublicKey(keys.privateKey));
    if (public_key != keys.publicKey) {
      return KeyValidatorError::KEYS_DONT_MATCH;
    }

    return outcome::success();
  }

  outcome::result<void> KeyValidatorImpl::validateRsa(
      const PrivateKey &key) const {
    const unsigned char *data_pointer = key.data.data();
    RSA *rsa = d2i_RSAPrivateKey(nullptr, &data_pointer, key.data.size());
    if (nullptr == rsa) {
      return KeyValidatorError::INVALID_PRIVATE_KEY;
    }
    auto cleanup_rsa = gsl::finally([rsa]() { RSA_free(rsa); });
    int bits = RSA_bits(rsa);
    if (bits != getRsaBitsCount(key.type)) {
      return KeyValidatorError::INVALID_PRIVATE_KEY;
    }

    return outcome::success();
  }

  outcome::result<void> KeyValidatorImpl::validateRsa(
      const PublicKey &key) const {
    const unsigned char *data_pointer = key.data.data();
    RSA *rsa = d2i_RSAPublicKey(nullptr, &data_pointer, key.data.size());
    if (nullptr == rsa) {
      return KeyValidatorError::INVALID_PUBLIC_KEY;
    }
    auto cleanup_rsa = gsl::finally([rsa]() { RSA_free(rsa); });
    int bits = RSA_bits(rsa);
    if (bits != getRsaBitsCount(key.type)) {
      return KeyValidatorError::INVALID_PRIVATE_KEY;
    }

    return outcome::success();
  }

  outcome::result<void> KeyValidatorImpl::validateEd25519(
      const PrivateKey &key) const {
    if (key.data.size() != ED25519_PRIVATE_KEY_SIZE) {
      return KeyValidatorError::WRONG_PRIVATE_KEY_SIZE;
    }

    return outcome::success();
  }

  outcome::result<void> KeyValidatorImpl::validateEd25519(
      const PublicKey &key) const {
    if (key.data.size() != ED25519_PUBLIC_KEY_SIZE) {
      return KeyValidatorError::WRONG_PUBLIC_KEY_SIZE;
    }

    return outcome::success();
  }

  outcome::result<void> KeyValidatorImpl::validateSecp256k1(
      const PrivateKey &key) const {
    if (key.data.size() != SECP256K1_PRIVATE_KEY_SIZE) {
      return KeyValidatorError::WRONG_PRIVATE_KEY_SIZE;
    }

    return outcome::success();
  }

  outcome::result<void> KeyValidatorImpl::validateSecp256k1(
      const PublicKey &key) const {
    if (key.data.size() != SECP256K1_PUBLIC_KEY_SIZE) {
      return KeyValidatorError::WRONG_PRIVATE_KEY_SIZE;
    }

    auto prefix = key.data[0];

    // compressed form of public secp256k1 key
    // must start with 0x02 or 0x03 byte prefix
    if (prefix != SECP256K1_PUBLIC_KEY_EVEN_PREFIX
        && prefix != SECP256K1_PUBLIC_KEY_ODD_PREFIX) {
      return KeyValidatorError::INVALID_PUBLIC_KEY;
    }

    return outcome::success();
  }

}  // namespace libp2p::crypto::validator
