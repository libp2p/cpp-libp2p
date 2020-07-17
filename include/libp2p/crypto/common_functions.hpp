/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_FUNCTIONS_HPP
#define LIBP2P_COMMON_FUNCTIONS_HPP

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <gsl/span>
#include <libp2p/outcome/outcome.hpp>

/*
 *  auto pkv =
      "6d8e72d53e0f8582f52169bf7f6c60ddb7e0fbb83af97a11cff02f1bf21bbf7c"_unhex;

  PrivateKey private_key;
  std::copy_n(pkv.begin(), private_key.size(), private_key.begin());

  EXPECT_OUTCOME_TRUE(public_key, provider.derive(private_key));
  std::vector<uint8_t> pubv;
  pubv.resize(public_key.size(), 0);
  std::copy_n(public_key.begin(), public_key.size(), pubv.begin());
  ASSERT_EQ(
      pubv,
      "502d10724db25437888bcd8e3e473ae226cb746740c2bb67fab6a31c650cb228"_unhex);
 */

namespace libp2p::crypto {

  template <typename Array>
  std::vector<uint8_t> asVector(const Array &key) {
    std::vector<uint8_t> result;
    result.resize(key.size(), 0);
    std::copy_n(key.begin(), key.size(), result.begin());
    return result;
  }

  template <typename Array>
  Array asArray(const std::vector<uint8_t> &bytes) {
    Array key;
    std::copy_n(bytes.begin(), key.size(), key.begin());
    return key;
  }

  /**
   * Initializes EC_KEY structure with private and public key from private key
   * bytes
   * @param nid - curve group identifier for the key, ex. NID_secp256k1
   * @param private_key - private key bytes
   * @return shared pointer to EC_KEY with overridden destructor
   */
  outcome::result<std::shared_ptr<EC_KEY>> EcKeyFromPrivateKeyBytes(
      int nid, gsl::span<const uint8_t> private_key);

  /**
   * Initializes EVP_PKEY structure from private or public key bytes
   * @tparam ReaderFunc type of restore method. ex. EVP_PKEY_new_raw_private_key
   * or EVP_PKEY_new_raw_public_key
   * @param type - key type (almost the same as nid for ECC). ex.
   * EVP_PKEY_ED25519
   * @param key_bytes - raw representation of the source key
   * @param reader - pointer to a function of type ReaderFunc
   * @return shared pointer to EVP_PKEY with overridden destructor
   */
  template <typename ReaderFunc>
  outcome::result<std::shared_ptr<EVP_PKEY>> NewEvpPkeyFromBytes(
      int type, gsl::span<const uint8_t> key_bytes, ReaderFunc *reader);

  /*
   * The following template instantiation serves for both -
   * EVP_PKEY_new_raw_private_key and EVP_PKEY_new_raw_public_key
   * since their types are identical.
   */
  extern template outcome::result<std::shared_ptr<EVP_PKEY>>
  NewEvpPkeyFromBytes(int, gsl::span<const uint8_t>,
                      decltype(EVP_PKEY_new_raw_public_key) *);

  /**
   * @brief Generate EC signature based on key type
   * @param digest - message hash
   * @param key - private key
   * @param output - signature result
   * @return EC signature or error code
   */
  outcome::result<std::vector<uint8_t>> GenerateEcSignature(
      gsl::span<const uint8_t> digest, const std::shared_ptr<EC_KEY> &key);

  /**
   * @brief Verify EC signature based on key type
   * @param digest - message hash
   * @param signature - bytes of the EC signature
   * @param key - EC public key
   * @return signature status or error code
   */
  outcome::result<bool> VerifyEcSignature(gsl::span<const uint8_t> digest,
                                          gsl::span<const uint8_t> signature,
                                          const std::shared_ptr<EC_KEY> &key);

}  // namespace libp2p::crypto

#endif  // LIBP2P_COMMON_FUNCTIONS_HPP
