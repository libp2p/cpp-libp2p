/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/x25519_provider/x25519_provider_impl.hpp>

#include <openssl/evp.h>
#include <libp2p/crypto/common_functions.hpp>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto::x25519 {
  outcome::result<Keypair> X25519ProviderImpl::generate() const {
    constexpr auto FAILED{KeyGeneratorError::KEY_GENERATION_FAILED};

    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
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

  outcome::result<PublicKey> X25519ProviderImpl::derive(
      const PrivateKey &private_key) const {
    OUTCOME_TRY(evp_pkey,
                NewEvpPkeyFromBytes(EVP_PKEY_X25519, private_key,
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

  outcome::result<std::vector<uint8_t>> X25519ProviderImpl::dh(
      const PrivateKey &private_key, const PublicKey &public_key) const {
    constexpr auto FAILED{KeyGeneratorError::KEY_GENERATION_FAILED};
    OUTCOME_TRY(evp_pkey,
                NewEvpPkeyFromBytes(EVP_PKEY_X25519, private_key,
                                    EVP_PKEY_new_raw_private_key));
    OUTCOME_TRY(evp_peerkey,
                NewEvpPkeyFromBytes(EVP_PKEY_X25519, public_key,
                                    EVP_PKEY_new_raw_public_key));
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new(evp_pkey.get(), nullptr);
    if (nullptr == pctx) {
      return FAILED;
    }
    auto free_pctx = gsl::finally([pctx] { EVP_PKEY_CTX_free(pctx); });

    if (1 != EVP_PKEY_derive_init(pctx)) {
      return FAILED;
    }

    if (1 != EVP_PKEY_derive_set_peer(pctx, evp_peerkey.get())) {
      return FAILED;
    }
    size_t shared_secret_len{0};

    if (1 != EVP_PKEY_derive(pctx, nullptr, &shared_secret_len)) {
      return FAILED;
    }

    std::vector<uint8_t> shared_secret;
    shared_secret.resize(shared_secret_len, 0);

    if (1 != EVP_PKEY_derive(pctx, shared_secret.data(), &shared_secret_len)) {
      return FAILED;
    }

    return shared_secret;
  }
}  // namespace libp2p::crypto::x25519
