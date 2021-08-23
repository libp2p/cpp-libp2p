/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/common_functions.hpp>

#include <gsl/gsl_util>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto {

  outcome::result<std::shared_ptr<EC_KEY>> EcKeyFromPrivateKeyBytes(
      int nid, gsl::span<const uint8_t> private_key) {
    auto FAILED = KeyGeneratorError::INTERNAL_ERROR;

    /*
     * Allocate the key.
     * Here we use shared pointer instead of gsl::finally since this is going to
     * be used as a return value in case of success.
     */
    std::shared_ptr<EC_KEY> key{EC_KEY_new_by_curve_name(nid), EC_KEY_free};
    if (nullptr == key) {
      return FAILED;
    }

    // turn private key bytes into big number
    BIGNUM *private_bignum{BN_bin2bn(
        private_key.data(), static_cast<int>(private_key.size()), nullptr)};
    if (nullptr == private_bignum) {
      return FAILED;
    }
    auto free_private_bignum =
        gsl::finally([private_bignum] { BN_free(private_bignum); });

    // set private key to the resulting key structure
    if (1 != EC_KEY_set_private_key(key.get(), private_bignum)) {
      return FAILED;
    }

    // EC_KEY that has a private key but do not has public key assumed invalid
    // thus we are generating its public key part
    const EC_GROUP *group = EC_KEY_get0_group(key.get());
    EC_POINT *public_key_point{EC_POINT_new(group)};
    if (nullptr == public_key_point) {
      return FAILED;
    }
    auto free_public_key_point =
        gsl::finally([public_key_point] { EC_POINT_free(public_key_point); });

    // derive the public key
    if (1
        != EC_POINT_mul(EC_KEY_get0_group(key.get()), public_key_point,
                        private_bignum, nullptr, nullptr, nullptr)) {
      return FAILED;
    }

    // check the public key
    if (1
        != EC_POINT_is_on_curve(EC_KEY_get0_group(key.get()), public_key_point,
                                nullptr)) {
      return KeyValidatorError::INVALID_PUBLIC_KEY;
    }

    // set public key to the resulting key structure
    if (1 != EC_KEY_set_public_key(key.get(), public_key_point)) {
      return FAILED;
    }

    // final check
    if (1 != EC_KEY_check_key(key.get())) {
      return KeyValidatorError::INVALID_PRIVATE_KEY;
    }

    return key;
  }

  template <typename ReaderFunc>
  outcome::result<std::shared_ptr<EVP_PKEY>> NewEvpPkeyFromBytes(
      int type, gsl::span<const uint8_t> key_bytes, ReaderFunc *reader) {
    std::shared_ptr<EVP_PKEY> key{
        reader(type, nullptr, key_bytes.data(), key_bytes.size()),
        EVP_PKEY_free};

    if (nullptr == key) {
      return KeyGeneratorError::INTERNAL_ERROR;
    }
    return key;
  }

  template outcome::result<std::shared_ptr<EVP_PKEY>> NewEvpPkeyFromBytes(
      int, gsl::span<const uint8_t>, decltype(EVP_PKEY_new_raw_public_key) *);

  outcome::result<std::vector<uint8_t>> GenerateEcSignature(
      gsl::span<const uint8_t> digest, const std::shared_ptr<EC_KEY> &key) {
    std::shared_ptr<ECDSA_SIG> signature{
        ECDSA_do_sign(digest.data(), static_cast<int>(digest.size()),
                      key.get()),
        ECDSA_SIG_free};
    if (signature == nullptr) {
      return CryptoProviderError::SIGNATURE_GENERATION_FAILED;
    }
    int signature_length = i2d_ECDSA_SIG(signature.get(), nullptr);
    if (signature_length < 0) {
      return CryptoProviderError::SIGNATURE_GENERATION_FAILED;
    }
    std::vector<uint8_t> signature_bytes;
    signature_bytes.resize(signature_length);
    uint8_t *signature_bytes_ptr = signature_bytes.data();
    i2d_ECDSA_SIG(signature.get(), &signature_bytes_ptr);
    return std::move(signature_bytes);
  }

  outcome::result<bool> VerifyEcSignature(gsl::span<const uint8_t> digest,
                                          gsl::span<const uint8_t> signature,
                                          const std::shared_ptr<EC_KEY> &key) {
    int result = ECDSA_verify(0, digest.data(), static_cast<int>(digest.size()),
                              signature.data(),
                              static_cast<int>(signature.size()), key.get());
    if (result < 0) {
      return CryptoProviderError::SIGNATURE_VERIFICATION_FAILED;
    }
    return result == 1;
  }

}  // namespace libp2p::crypto
