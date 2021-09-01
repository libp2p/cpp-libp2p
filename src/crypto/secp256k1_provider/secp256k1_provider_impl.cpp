/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp"

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

#include <memory>

#include "libp2p/crypto/common_functions.hpp"
#include "libp2p/crypto/error.hpp"
#include "libp2p/crypto/sha/sha256.hpp"

namespace libp2p::crypto::secp256k1 {
  outcome::result<KeyPair> Secp256k1ProviderImpl::generate() const {
    PublicKey public_key{};
    PrivateKey private_key{};
    std::shared_ptr<EC_KEY> key{EC_KEY_new_by_curve_name(NID_secp256k1),
                                EC_KEY_free};
    if (EC_KEY_generate_key(key.get()) != 1) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    const BIGNUM *privateNum = EC_KEY_get0_private_key(key.get());
    if (BN_bn2binpad(privateNum, private_key.data(), private_key.size()) < 0) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    EC_KEY_set_conv_form(key.get(), POINT_CONVERSION_COMPRESSED);
    auto public_key_length = i2o_ECPublicKey(key.get(), nullptr);
    if (public_key_length != public_key.size()) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    uint8_t *public_key_ptr = public_key.data();
    if (i2o_ECPublicKey(key.get(), &public_key_ptr) != public_key_length) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    return KeyPair{private_key, public_key};
  }

  outcome::result<PublicKey> Secp256k1ProviderImpl::derive(
      const PrivateKey &key) const {
    OUTCOME_TRY(private_key, bytesToPrivateKey(key));
    PublicKey public_key{};
    uint8_t *public_key_ptr = public_key.data();
    ssize_t generated_length = i2o_ECPublicKey(private_key.get(), nullptr);
    if (generated_length != public_key.size()) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    i2o_ECPublicKey(private_key.get(), &public_key_ptr);
    return public_key;
  }

  outcome::result<Signature> Secp256k1ProviderImpl::sign(
      gsl::span<const uint8_t> message, const PrivateKey &key) const {
    OUTCOME_TRY(digest, sha256(message));
    OUTCOME_TRY(private_key, bytesToPrivateKey(key));
    OUTCOME_TRY(signature, GenerateEcSignature(digest, private_key));
    return std::move(signature);
  }

  outcome::result<bool> Secp256k1ProviderImpl::verify(
      gsl::span<const uint8_t> message, const Signature &signature,
      const PublicKey &key) const {
    OUTCOME_TRY(digest, sha256(message));
    OUTCOME_TRY(public_key, bytesToPublicKey(key));
    OUTCOME_TRY(result, VerifyEcSignature(digest, signature, public_key));
    return result;
  }

  outcome::result<std::shared_ptr<EC_KEY>>
  Secp256k1ProviderImpl::bytesToPrivateKey(const PrivateKey &input) {
    std::shared_ptr<EC_KEY> key{EC_KEY_new_by_curve_name(NID_secp256k1),
                                EC_KEY_free};
    if (key == nullptr) {
      return KeyGeneratorError::INTERNAL_ERROR;
    }
    std::unique_ptr<BIGNUM, void (*)(BIGNUM *)> key_bignum{
        BN_bin2bn(input.data(), static_cast<int>(input.size()), nullptr),
        BN_free};
    if (key_bignum == nullptr) {
      return KeyGeneratorError::INTERNAL_ERROR;
    }
    if (EC_KEY_set_private_key(key.get(), key_bignum.get()) != 1) {
      return KeyGeneratorError::INTERNAL_ERROR;
    }
    std::unique_ptr<EC_POINT, void (*)(EC_POINT *)> point{
        EC_POINT_new(EC_KEY_get0_group(key.get())), EC_POINT_free};
    if (point == nullptr) {
      return KeyGeneratorError::INTERNAL_ERROR;
    }
    EC_KEY_set_conv_form(key.get(), POINT_CONVERSION_COMPRESSED);
    if (EC_POINT_mul(EC_KEY_get0_group(key.get()), point.get(),
                     key_bignum.get(), nullptr, nullptr, nullptr)
        != 1) {
      return KeyGeneratorError::INTERNAL_ERROR;
    }
    if (EC_KEY_set_public_key(key.get(), point.get()) != 1) {
      return KeyGeneratorError::INTERNAL_ERROR;
    }
    if (EC_KEY_check_key(key.get()) != 1) {
      return KeyValidatorError::INVALID_PRIVATE_KEY;
    }
    return key;
  }

  outcome::result<std::shared_ptr<EC_KEY>>
  Secp256k1ProviderImpl::bytesToPublicKey(const PublicKey &input) {
    std::shared_ptr<EC_KEY> key{EC_KEY_new_by_curve_name(NID_secp256k1),
                                EC_KEY_free};
    if (key == nullptr) {
      return KeyGeneratorError::INTERNAL_ERROR;
    }
    const uint8_t *public_key_ptr = input.data();
    EC_KEY *key_ptr = key.get();
    key_ptr = o2i_ECPublicKey(&key_ptr, &public_key_ptr,
                              static_cast<long>(input.size()));
    if (key_ptr == nullptr) {
      return KeyGeneratorError::INTERNAL_ERROR;
    }
    if (EC_KEY_check_key(key.get()) != 1) {
      return KeyValidatorError::INVALID_PUBLIC_KEY;
    }
    return key;
  }
}  // namespace libp2p::crypto::secp256k1
