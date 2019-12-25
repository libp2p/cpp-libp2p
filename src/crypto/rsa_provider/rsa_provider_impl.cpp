/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/rsa_provider/rsa_provider_impl.hpp"

#include "libp2p/crypto/sha/sha256.hpp"
#include <memory>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

using Hash256 = libp2p::common::Hash256;

namespace libp2p::crypto::rsa {

  outcome::result<std::shared_ptr<RSA>> rsaFromPrivateKey(
      const PrivateKey &private_key) {
    const unsigned char *data_pointer = private_key.data();
    std::shared_ptr<RSA> rsa{
        d2i_RSAPrivateKey(nullptr, &data_pointer, private_key.size()),
        RSA_free};
    if (nullptr == rsa) {
      return KeyValidatorError::INVALID_PRIVATE_KEY;
    }
    return rsa;
  }

  outcome::result<Signature> RsaProviderImpl::sign(
      gsl::span<uint8_t> message, const PrivateKey &private_key) const {
    OUTCOME_TRY(rsa, rsaFromPrivateKey(private_key));
    Hash256 digest = sha256(message);
    Signature signature(RSA_size(rsa.get()));
    unsigned int signature_size;
    if (1
        != RSA_sign(NID_sha256, digest.data(), digest.size(), signature.data(),
                    &signature_size, rsa.get())) {
      return CryptoProviderError::SIGNATURE_GENERATION_FAILED;
    }

    return signature;
  }

  outcome::result<bool> RsaProviderImpl::verify(gsl::span<uint8_t> message,
                                                const Signature &signature,
                                                const PublicKey &public_key) const {
    OUTCOME_TRY(x509_key, RsaProviderImpl::getPublicKeyFromBytes(public_key));
    EVP_PKEY *key = X509_PUBKEY_get0(x509_key.get());
    std::unique_ptr<RSA, void (*)(RSA *)> rsa{EVP_PKEY_get1_RSA(key), RSA_free};
    Hash256 digest = sha256(message);
    int result = RSA_verify(NID_sha256, digest.data(), digest.size(),
                            signature.data(), signature.size(), rsa.get());
    return result == 1;
  }

  outcome::result<std::shared_ptr<X509_PUBKEY>>
  RsaProviderImpl::getPublicKeyFromBytes(const PublicKey &input_key) {
    const uint8_t *bytes = input_key.data();
    std::shared_ptr<X509_PUBKEY> key{X509_PUBKEY_new(), X509_PUBKEY_free};
    X509_PUBKEY *key_ptr = key.get();
    if (d2i_X509_PUBKEY(&key_ptr, &bytes, input_key.size()) == nullptr) {
      return KeyValidatorError::INVALID_PUBLIC_KEY;
    }
    return key;
  }

};  // namespace libp2p::crypto::rsa
