/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/crypto_provider/crypto_provider_impl.hpp>

#include <exception>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <gsl/gsl_util>
#include <gsl/pointers>
#include <gsl/span>
#include <libp2p/crypto/ecdsa_provider.hpp>
#include <libp2p/crypto/ed25519_provider.hpp>
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/random_generator.hpp>
#include <libp2p/crypto/rsa_provider.hpp>
#include <libp2p/crypto/secp256k1_provider.hpp>

namespace libp2p::crypto {

  CryptoProviderImpl::CryptoProviderImpl(
      std::shared_ptr<random::CSPRNG> random_provider,
      std::shared_ptr<ed25519::Ed25519Provider> ed25519_provider,
      std::shared_ptr<rsa::RsaProvider> rsa_provider,
      std::shared_ptr<ecdsa::EcdsaProvider> ecdsa_provider,
      std::shared_ptr<secp256k1::Secp256k1Provider> secp256k1_provider)
      : random_provider_{std::move(random_provider)},
        ed25519_provider_{std::move(ed25519_provider)},
        rsa_provider_{std::move(rsa_provider)},
        ecdsa_provider_{std::move(ecdsa_provider)},
        secp256k1_provider_{std::move(secp256k1_provider)} {
    initialize();
  }

  void CryptoProviderImpl::initialize() {
    constexpr size_t kSeedBytesCount = 128 * 4;  // ripple uses such number
    auto bytes = random_provider_->randomBytes(kSeedBytesCount);
    // seeding random crypto_provider is required prior to calling
    // RSA_generate_key NOLINTNEXTLINE
    RAND_seed(static_cast<const void *>(bytes.data()), bytes.size());
  }

  /* ###################################################################
   *
   * Key Generation
   *
   * ################################################################### */

  outcome::result<KeyPair> CryptoProviderImpl::generateKeys(
      Key::Type key_type, common::RSAKeyType rsa_bitness) const {
    switch (key_type) {
      case Key::Type::RSA:
        return generateRsa(rsa_bitness);
      case Key::Type::Ed25519:
        return generateEd25519();
      case Key::Type::Secp256k1:
        return generateSecp256k1();
      case Key::Type::ECDSA:
        return generateEcdsa();
      default:
        return KeyGeneratorError::UNSUPPORTED_KEY_TYPE;
    }
  }

  outcome::result<KeyPair> CryptoProviderImpl::generateRsa(
      common::RSAKeyType rsa_bitness) const {
    OUTCOME_TRY(rsa, rsa_provider_->generate(rsa_bitness));

    auto &&pub = rsa.public_key;
    auto &&priv = rsa.private_key;
    return KeyPair{.publicKey = {{.type = Key::Type::RSA,
                                  .data = {pub.begin(), pub.end()}}},
                   .privateKey = {{.type = Key::Type::RSA,
                                   .data = {priv.begin(), priv.end()}}}};
  }

  outcome::result<KeyPair> CryptoProviderImpl::generateEd25519() const {
    OUTCOME_TRY(ed, ed25519_provider_->generate());

    auto &&pub = ed.public_key;
    auto &&priv = ed.private_key;
    return KeyPair{.publicKey = {{.type = Key::Type::Ed25519,
                                  .data = {pub.begin(), pub.end()}}},
                   .privateKey = {{.type = Key::Type::Ed25519,
                                   .data = {priv.begin(), priv.end()}}}};
  }

  outcome::result<KeyPair> CryptoProviderImpl::generateSecp256k1() const {
    OUTCOME_TRY(secp, secp256k1_provider_->generate());

    auto &&pub = secp.public_key;
    auto &&priv = secp.private_key;
    return KeyPair{.publicKey = {{.type = Key::Type::Secp256k1,
                                  .data = {pub.begin(), pub.end()}}},
                   .privateKey = {{.type = Key::Type::Secp256k1,
                                   .data = {priv.begin(), priv.end()}}}};
  }

  outcome::result<KeyPair> CryptoProviderImpl::generateEcdsa() const {
    OUTCOME_TRY(ecdsa, ecdsa_provider_->generate());

    auto &&pub = ecdsa.public_key;
    auto &&priv = ecdsa.private_key;
    return KeyPair{.publicKey = {{.type = Key::Type::ECDSA,
                                  .data = {pub.begin(), pub.end()}}},
                   .privateKey = {{.type = Key::Type::ECDSA,
                                   .data = {priv.begin(), priv.end()}}}};
  }

  /* ###################################################################
   *
   * Public Key Derivation
   *
   * ################################################################### */

  outcome::result<PublicKey> CryptoProviderImpl::derivePublicKey(
      const PrivateKey &private_key) const {
    switch (private_key.type) {
      case Key::Type::RSA:
        return deriveRsa(private_key);
      case Key::Type::Ed25519:
        return deriveEd25519(private_key);
      case Key::Type::Secp256k1:
        return deriveSecp256k1(private_key);
      case Key::Type::ECDSA:
        return deriveEcdsa(private_key);
      case Key::Type::UNSPECIFIED:
        return KeyGeneratorError::WRONG_KEY_TYPE;
    }
    return KeyGeneratorError::UNSUPPORTED_KEY_TYPE;
  }

  outcome::result<PublicKey> CryptoProviderImpl::deriveRsa(
      const PrivateKey &key) const {
    rsa::PrivateKey private_key;
    private_key.insert(private_key.end(), key.data.begin(), key.data.end());
    OUTCOME_TRY(rsa_pub, rsa_provider_->derive(private_key));
    return PublicKey{{key.type, {rsa_pub.begin(), rsa_pub.end()}}};
  }

  outcome::result<PublicKey> CryptoProviderImpl::deriveEd25519(
      const PrivateKey &key) const {
    ed25519::PrivateKey private_key;
    std::copy_n(key.data.begin(), private_key.size(), private_key.begin());
    OUTCOME_TRY(ed_pub, ed25519_provider_->derive(private_key));

    return PublicKey{{key.type, {ed_pub.begin(), ed_pub.end()}}};
  }

  outcome::result<PublicKey> CryptoProviderImpl::deriveSecp256k1(
      const PrivateKey &key) const {
    secp256k1::PrivateKey private_key;
    std::copy_n(key.data.begin(), private_key.size(), private_key.begin());
    OUTCOME_TRY(secp_pub, secp256k1_provider_->derive(private_key));

    return PublicKey{{key.type, {secp_pub.begin(), secp_pub.end()}}};
  }

  outcome::result<PublicKey> CryptoProviderImpl::deriveEcdsa(
      const PrivateKey &key) const {
    ecdsa::PrivateKey private_key;
    std::copy_n(key.data.begin(), private_key.size(), private_key.begin());
    OUTCOME_TRY(ecdsa_pub, ecdsa_provider_->derive(private_key));

    return PublicKey{{key.type, {ecdsa_pub.begin(), ecdsa_pub.end()}}};
  }

  /* ###################################################################
   *
   * Messages Signing
   *
   * ################################################################### */

  outcome::result<Buffer> CryptoProviderImpl::sign(
      gsl::span<const uint8_t> message, const PrivateKey &private_key) const {
    switch (private_key.type) {
      case Key::Type::RSA:
        return signRsa(message, private_key);
      case Key::Type::Ed25519:
        return signEd25519(message, private_key);
      case Key::Type::Secp256k1:
        return signSecp256k1(message, private_key);
      case Key::Type::ECDSA:
        return signEcdsa(message, private_key);
      case Key::Type::UNSPECIFIED:
        return KeyGeneratorError::WRONG_KEY_TYPE;
      default:
        return CryptoProviderError::SIGNATURE_GENERATION_FAILED;
    }
  }

  outcome::result<Buffer> CryptoProviderImpl::signRsa(
      gsl::span<const uint8_t> message, const PrivateKey &private_key) const {
    rsa::PrivateKey priv_key;
    priv_key.insert(priv_key.end(), private_key.data.begin(),
                    private_key.data.end());

    OUTCOME_TRY(signature, rsa_provider_->sign(message, priv_key));
    return {signature.begin(), signature.end()};
  }

  outcome::result<Buffer> CryptoProviderImpl::signEd25519(
      gsl::span<const uint8_t> message, const PrivateKey &private_key) const {
    ed25519::PrivateKey priv_key;
    std::copy_n(private_key.data.begin(), priv_key.size(), priv_key.begin());
    OUTCOME_TRY(signature, ed25519_provider_->sign(message, priv_key));
    return {signature.begin(), signature.end()};
  }

  outcome::result<Buffer> CryptoProviderImpl::signSecp256k1(
      gsl::span<const uint8_t> message, const PrivateKey &private_key) const {
    secp256k1::PrivateKey priv_key;
    std::copy_n(private_key.data.begin(), priv_key.size(), priv_key.begin());
    OUTCOME_TRY(signature, secp256k1_provider_->sign(message, priv_key));
    return {signature.begin(), signature.end()};
  }

  outcome::result<Buffer> CryptoProviderImpl::signEcdsa(
      gsl::span<const uint8_t> message, const PrivateKey &private_key) const {
    ecdsa::PrivateKey priv_key;
    std::copy_n(private_key.data.begin(), priv_key.size(), priv_key.begin());
    OUTCOME_TRY(signature, ecdsa_provider_->sign(message, priv_key));
    return {signature.begin(), signature.end()};
  }

  /* ###################################################################
   *
   * Signatures Verifying
   *
   * ################################################################### */

  outcome::result<bool> CryptoProviderImpl::verify(
      gsl::span<const uint8_t> message, gsl::span<const uint8_t> signature,
      const PublicKey &public_key) const {
    switch (public_key.type) {
      case Key::Type::RSA:
        return verifyRsa(message, signature, public_key);
      case Key::Type::Ed25519:
        return verifyEd25519(message, signature, public_key);
      case Key::Type::Secp256k1:
        return verifySecp256k1(message, signature, public_key);
      case Key::Type::ECDSA:
        return verifyEcdsa(message, signature, public_key);
      case Key::Type::UNSPECIFIED:
        return KeyGeneratorError::WRONG_KEY_TYPE;
      default:
        return CryptoProviderError::SIGNATURE_VERIFICATION_FAILED;
    }
  }

  outcome::result<bool> CryptoProviderImpl::verifyRsa(
      gsl::span<const uint8_t> message, gsl::span<const uint8_t> signature,
      const PublicKey &public_key) const {
    rsa::PublicKey rsa_pub;
    rsa_pub.insert(rsa_pub.end(), public_key.data.begin(),
                   public_key.data.end());

    rsa::Signature rsa_sig;
    rsa_sig.insert(rsa_sig.end(), signature.begin(), signature.end());

    return rsa_provider_->verify(message, rsa_sig, rsa_pub);
  }

  outcome::result<bool> CryptoProviderImpl::verifyEd25519(
      gsl::span<const uint8_t> message, gsl::span<const uint8_t> signature,
      const PublicKey &public_key) const {
    ed25519::PublicKey ed_pub;
    std::copy_n(public_key.data.begin(), ed_pub.size(), ed_pub.begin());

    ed25519::Signature ed_sig;
    std::copy_n(signature.begin(), ed_sig.size(), ed_sig.begin());

    return ed25519_provider_->verify(message, ed_sig, ed_pub);
  }

  outcome::result<bool> CryptoProviderImpl::verifySecp256k1(
      gsl::span<const uint8_t> message, gsl::span<const uint8_t> signature,
      const PublicKey &public_key) const {
    secp256k1::PublicKey secp_pub;
    std::copy_n(public_key.data.begin(), secp_pub.size(), secp_pub.begin());

    ecdsa::Signature secp_sig;
    secp_sig.insert(secp_sig.end(), signature.begin(), signature.end());

    return secp256k1_provider_->verify(message, secp_sig, secp_pub);
  }

  outcome::result<bool> CryptoProviderImpl::verifyEcdsa(
      gsl::span<const uint8_t> message, gsl::span<const uint8_t> signature,
      const PublicKey &public_key) const {
    ecdsa::PublicKey ecdsa_pub;
    std::copy_n(public_key.data.begin(), ecdsa_pub.size(), ecdsa_pub.begin());

    ecdsa::Signature ecdsa_sig;
    ecdsa_sig.insert(ecdsa_sig.end(), signature.begin(), signature.end());

    return ecdsa_provider_->verify(message, ecdsa_sig, ecdsa_pub);
  }

  /* ###################################################################
   *
   * Ephemeral Key Cryptography
   *
   * ################################################################### */

  outcome::result<EphemeralKeyPair>
  CryptoProviderImpl::generateEphemeralKeyPair(common::CurveType curve) const {
    // TODO(yuraz): pre-140 implement
    return KeyGeneratorError::KEY_GENERATION_FAILED;
  }

  std::vector<StretchedKey> CryptoProviderImpl::stretchKey(
      common::CipherType cipher_type, common::HashType hash_type,
      const Buffer &secret) const {
    // TODO(yuraz): pre-140 implement
    return {};
  }
}  // namespace libp2p::crypto
