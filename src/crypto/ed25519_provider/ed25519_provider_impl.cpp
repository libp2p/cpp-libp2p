/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>

#include <openssl/evp.h>
#include <libp2p/crypto/common_functions.hpp>
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/sha/sha512.hpp>

namespace libp2p::crypto::ed25519 {

  outcome::result<Keypair> Ed25519ProviderImpl::generate() const {
    constexpr auto FAILED{KeyGeneratorError::KEY_GENERATION_FAILED};

    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    if (nullptr == pctx) {
      return FAILED;
    }
    auto free_pctx = gsl::finally([pctx] { EVP_PKEY_CTX_free(pctx); });

    if (1 != EVP_PKEY_keygen_init(pctx)) {
      return FAILED;
    }

    EVP_PKEY *pkey{nullptr};  // it is mandatory to nullify the pointer!
    if (1 != EVP_PKEY_keygen(pctx, &pkey)) {
      return FAILED;
    }
    auto free_pkey = gsl::finally([pkey] { EVP_PKEY_free(pkey); });

    Keypair keypair{};
    size_t priv_len{keypair.private_key.size()};
    size_t pub_len{keypair.public_key.size()};
    if (1
        != EVP_PKEY_get_raw_private_key(pkey, keypair.private_key.data(),
                                        &priv_len)) {
      return FAILED;
    }
    if (1
        != EVP_PKEY_get_raw_public_key(pkey, keypair.public_key.data(),
                                       &pub_len)) {
      return FAILED;
    }

    return keypair;
  }

  outcome::result<PublicKey> Ed25519ProviderImpl::derive(
      const PrivateKey &private_key) const {
    OUTCOME_TRY(evp_pkey,
                NewEvpPkeyFromBytes(EVP_PKEY_ED25519, private_key,
                                    EVP_PKEY_new_raw_private_key));
    PublicKey public_key{0};
    size_t pub_len{public_key.size()};
    if (1
        != EVP_PKEY_get_raw_public_key(evp_pkey.get(), public_key.data(),
                                       &pub_len)) {
      return KeyGeneratorError::KEY_DERIVATION_FAILED;
    }

    return public_key;
  }

  outcome::result<Signature> Ed25519ProviderImpl::sign(
      gsl::span<const uint8_t> message, const PrivateKey &private_key) const {
    OUTCOME_TRY(evp_pkey,
                NewEvpPkeyFromBytes(EVP_PKEY_ED25519, private_key,
                                    EVP_PKEY_new_raw_private_key));
    constexpr auto FAILED{CryptoProviderError::SIGNATURE_GENERATION_FAILED};

    std::shared_ptr<EVP_MD_CTX> mctx{EVP_MD_CTX_new(), EVP_MD_CTX_free};
    if (nullptr == mctx) {
      return FAILED;
    }

    if (1
        != EVP_DigestSignInit(mctx.get(), nullptr, nullptr, nullptr,
                              evp_pkey.get())) {
      return FAILED;
    }

    Signature signature;
    size_t signature_len{signature.size()};
    if (1
        != EVP_DigestSign(mctx.get(), signature.data(), &signature_len,
                          message.data(), message.size())) {
      return FAILED;
    }
    return signature;
  }

  outcome::result<bool> Ed25519ProviderImpl::verify(
      gsl::span<const uint8_t> message, const Signature &signature,
      const PublicKey &public_key) const {
    OUTCOME_TRY(evp_pkey,
                NewEvpPkeyFromBytes(EVP_PKEY_ED25519, public_key,
                                    EVP_PKEY_new_raw_public_key));
    constexpr auto FAILED{CryptoProviderError::SIGNATURE_VERIFICATION_FAILED};

    std::shared_ptr<EVP_MD_CTX> mctx{EVP_MD_CTX_new(), EVP_MD_CTX_free};
    if (nullptr == mctx) {
      return FAILED;
    }

    if (1
        != EVP_DigestVerifyInit(mctx.get(), nullptr, nullptr, nullptr,
                                evp_pkey.get())) {
      return FAILED;
    }
    int valid = EVP_DigestVerify(mctx.get(), signature.data(), signature.size(),
                                 message.data(), message.size());
    if (1 == valid || 0 == valid) {
      return valid;
    }

    return FAILED;
  }
}  // namespace libp2p::crypto::ed25519
