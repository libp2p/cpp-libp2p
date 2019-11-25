/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/crypto_provider/crypto_provider_impl.hpp>

#include <exception>
#include <iostream>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <gsl/gsl_util>
#include <gsl/pointers>
#include <gsl/span>
#include <libp2p/crypto/ed25519_provider.hpp>
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/random_generator.hpp>

namespace libp2p::crypto {
  CryptoProviderImpl::CryptoProviderImpl(
      std::shared_ptr<random::CSPRNG> random_provider,
      std::shared_ptr<ed25519::Ed25519Provider> ed25519_provider)
      : random_provider_{std::move(random_provider)},
        ed25519_provider_{std::move(ed25519_provider)} {
    initialize();
  }

  void CryptoProviderImpl::initialize() {
    constexpr size_t kSeedBytesCount = 128 * 4;  // ripple uses such number
    auto bytes = random_provider_->randomBytes(kSeedBytesCount);
    // seeding random crypto_provider is required prior to calling
    // RSA_generate_key NOLINTNEXTLINE
    RAND_seed(static_cast<const void *>(bytes.data()), bytes.size());
  }

  namespace detail {
    // encode cryptographic key in `ASN.1 DER` format
    template <class KeyStructure, class Function>
    outcome::result<std::vector<uint8_t>> encodeKeyDer(KeyStructure *ks,
                                                       Function *function) {
      unsigned char *buffer = nullptr;
      auto cleanup = gsl::finally([pptr = &buffer]() {
        if (*pptr != nullptr) {
          OPENSSL_free(*pptr);
        }
      });

      int length = function(ks, &buffer);
      if (length < 0) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      auto span = gsl::make_span(buffer, length);
      return std::vector<uint8_t>{span.begin(), span.end()};
    }

    // get private or public key of EVP_PKEY raw bytes in uniform way
    template <class Function>
    outcome::result<std::vector<uint8_t>> getEvpPkeyRawBytes(EVP_PKEY *pkey,
                                                             Function *funptr) {
      size_t len = 0;
      // get length
      if (1 != funptr(pkey, nullptr, &len)) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }
      // serialize
      std::vector<uint8_t> key_bytes(len, 0);
      if (1 != funptr(pkey, key_bytes.data(), &len)) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      return key_bytes;
    }

    outcome::result<PublicKey> deriveRsa(const PrivateKey &key) {
      const unsigned char *data_pointer = key.data.data();
      RSA *rsa = d2i_RSAPrivateKey(nullptr, &data_pointer, key.data.size());
      if (nullptr == rsa) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      auto cleanup_rsa = gsl::finally([rsa]() { RSA_free(rsa); });

      OUTCOME_TRY(public_bytes, detail::encodeKeyDer(rsa, i2d_RSA_PUBKEY));

      return PublicKey{{key.type, std::move(public_bytes)}};
    }

    outcome::result<PublicKey> deriveEcdsa256WithCurve(const PrivateKey &key,
                                                       int curve_nid) {
      // put private key bytes to internal representation

      // create new bignum for storing private key
      BIGNUM *private_bignum = BN_new();
      if (nullptr == private_bignum) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      // auto cleanup for bignum
      auto cleanup_bignum =
          gsl::finally([private_bignum]() { BN_free(private_bignum); });
      // convert private key bytes to BIGNUM
      const unsigned char *buffer = key.data.data();
      if (nullptr == BN_bin2bn(buffer, key.data.size(), private_bignum)) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }

      // generate public key from private using curve

      // get curve group
      EC_GROUP *group = EC_GROUP_new_by_curve_name(curve_nid);
      if (nullptr == group) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      auto cleanup_group = gsl::finally([group]() { EC_GROUP_free(group); });

      EC_POINT *point = EC_POINT_new(group);
      if (nullptr == point) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      auto cleanup_point = gsl::finally([point]() { EC_POINT_free(point); });
      // make public key
      if (1
          != EC_POINT_mul(group, point, private_bignum, nullptr, nullptr,
                          nullptr)) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }

      uint8_t *data_pointer = nullptr;
      auto cleanup_data = gsl::finally([pptr = &data_pointer]() {
        if (*pptr != nullptr) {
          OPENSSL_free(*pptr);
        }
      });

      int public_length = EC_POINT_point2buf(
          group, point, POINT_CONVERSION_COMPRESSED, &data_pointer, nullptr);
      if (public_length == 0) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      auto span = gsl::span(data_pointer, public_length);
      std::vector<uint8_t> public_buffer(span.begin(), span.end());

      return PublicKey{{key.type, std::move(public_buffer)}};
    }

    outcome::result<PublicKey> deriveSecp256k1(const PrivateKey &key) {
      return deriveEcdsa256WithCurve(key, NID_secp256k1);
    }

    outcome::result<PublicKey> deriveEcdsa(const PrivateKey &key) {
      return deriveEcdsa256WithCurve(key, NID_X9_62_prime256v1);
    }

    /**
     * according to libp2p specification:
     *  https://github.com/libp2p/specs/blob/master/peer-ids/peer-ids.md#how-keys-are-encoded-and-messages-signed
     *  We encode the public key using the DER-encoded PKIX format
     *  We encode the private key as a PKCS1 key using ASN.1 DER.
     *
     *  according to openssl manual:
     *  https://www.openssl.org/docs/man1.1.1/man3/i2d_RSA_PUBKEY.html
     *  d2i_RSA_PUBKEY() and i2d_RSA_PUBKEY() decode and encode a PKIX
     *
     *  https://www.openssl.org/docs/man1.1.1/man3/i2d_RSAPrivateKey.html
     *  d2i_RSAPrivateKey(), i2d_RSAPrivateKey() decode and encode a PKCS#1
     */
    outcome::result<std::pair<CryptoProvider::Buffer, CryptoProvider::Buffer>>
    generateRsaKeys(int bits) {
      int ret = 0;
      RSA *rsa = nullptr;
      BIGNUM *bne = nullptr;
      // clean up automatically
      auto cleanup = gsl::finally([&]() {
        if (nullptr != rsa) {
          RSA_free(rsa);
        }
        if (nullptr != bne) {
          BN_free(bne);
        }
      });

      constexpr uint64_t exp = RSA_F4;

      // 1. generate rsa state
      bne = BN_new();
      ret = BN_set_word(bne, exp);
      if (ret != 1) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      // 2. generate keys
      rsa = RSA_new();
      ret = RSA_generate_key_ex(rsa, bits, bne, nullptr);
      if (ret != 1) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      OUTCOME_TRY(public_bytes, detail::encodeKeyDer(rsa, i2d_RSA_PUBKEY));
      OUTCOME_TRY(private_bytes, detail::encodeKeyDer(rsa, i2d_RSAPrivateKey));

      return {std::move(public_bytes), std::move(private_bytes)};
    }
  }  // namespace detail

  outcome::result<KeyPair> CryptoProviderImpl::generateKeys(
      Key::Type key_type) const {
    switch (key_type) {
      case Key::Type::RSA:
        // TODO(akvinikym) 01.10.19 PRE-314: implement
        return KeyGeneratorError::KEY_GENERATION_FAILED;
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

  /// previous implementation is commented - it can be used as a hint when
  /// implementing a new version of the method
  //  outcome::result<KeyPair> CryptoProviderImpl::generateRsa(
  //      common::RSAKeyType bits_option) const {
  //    BOOST_ASSERT_MSG(false, "not implemented");
  //
  //    int bits = 0;
  //    Key::Type key_type;
  //    switch (bits_option) {
  //      case common::RSAKeyType::RSA1024:
  //        bits = 1024;
  //        key_type = Key::Type::RSA1024;
  //        break;
  //      case common::RSAKeyType::RSA2048:
  //        bits = 2048;
  //        key_type = Key::Type::RSA2048;
  //        break;
  //      case common::RSAKeyType::RSA4096:
  //        bits = 4096;
  //        key_type = Key::Type::RSA4096;
  //        break;
  //    }
  //
  //    OUTCOME_TRY(keys, detail::generateRsaKeys(bits));
  //
  //    return KeyPair{{{key_type, std::move(keys.first)}},
  //                   {{key_type, std::move(keys.second)}}};
  //  }

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
    return generateEcdsa256WithCurve(Key::Type::Secp256k1, NID_secp256k1);
  }

  outcome::result<KeyPair> CryptoProviderImpl::generateEcdsa() const {
    return generateEcdsa256WithCurve(Key::Type::ECDSA, NID_X9_62_prime256v1);
  }

  outcome::result<KeyPair> CryptoProviderImpl::generateEcdsa256WithCurve(
      Key::Type key_type, int curve_nid) const {
    EC_KEY *key = EC_KEY_new();
    if (nullptr == key) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    // auto free key structure
    auto free_key = gsl::finally([key]() {
      if (key != nullptr) {
        EC_KEY_free(key);
      }
    });

    // get curve group
    EC_GROUP *group = EC_GROUP_new_by_curve_name(curve_nid);
    if (nullptr == group) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    auto free_group = gsl::finally([group]() { EC_GROUP_free(group); });
    // set group
    if (1 != EC_KEY_set_group(key, group)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    // generate key pair
    if (1 != EC_KEY_generate_key(key)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    // get private key as bignum
    const BIGNUM *private_bignum = EC_KEY_get0_private_key(key);
    constexpr size_t PRIVATE_KEY_LENGTH = 32;
    std::vector<uint8_t> private_bytes(PRIVATE_KEY_LENGTH, 0);
    // serialize to buffer
    if (BN_bn2binpad(private_bignum, private_bytes.data(), PRIVATE_KEY_LENGTH)
        < 0) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    constexpr size_t PUBLIC_COMPRESSED_KEY_LENGTH = 33;
    // use compressed form of ecdsa public key
    EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
    // get buffer length for serializing public key
    auto public_length = i2o_ECPublicKey(key, nullptr);
    if (public_length != PUBLIC_COMPRESSED_KEY_LENGTH) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    std::vector<uint8_t> public_bytes(public_length, 0);
    // the `i2o_ECPublicKey` function modifies input pointer
    // so we need lvalue pointer to buffer
    uint8_t *public_pointer = public_bytes.data();
    // serialize public key
    if (i2o_ECPublicKey(key, &public_pointer) != public_length) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    return KeyPair{{{key_type, std::move(public_bytes)}},
                   {{key_type, std::move(private_bytes)}}};
  }

  outcome::result<PublicKey> CryptoProviderImpl::derivePublicKey(
      const PrivateKey &private_key) const {
    switch (private_key.type) {
      case Key::Type::RSA:
        return detail::deriveRsa(private_key);
      case Key::Type::Ed25519:
        return deriveEd25519(private_key);
      case Key::Type::Secp256k1:
        return detail::deriveSecp256k1(private_key);
      case Key::Type::ECDSA:
        return detail::deriveEcdsa(private_key);
      case Key::Type::UNSPECIFIED:
        return KeyGeneratorError::WRONG_KEY_TYPE;
    }
    return KeyGeneratorError::UNSUPPORTED_KEY_TYPE;
  }

  outcome::result<PublicKey> CryptoProviderImpl::deriveEd25519(
      const PrivateKey &key) const {
    ed25519::PrivateKey private_key;
    std::copy_n(key.data.begin(), private_key.size(), private_key.begin());
    OUTCOME_TRY(ed_pub, ed25519_provider_->derive(private_key));

    return PublicKey{{key.type, {ed_pub.begin(), ed_pub.end()}}};
  }

  outcome::result<Buffer> CryptoProviderImpl::sign(
      gsl::span<uint8_t> message, const PrivateKey &private_key) const {
    switch (private_key.type) {
      case Key::Type::RSA:
        return CryptoProviderError::SIGNATURE_GENERATION_FAILED;
      case Key::Type::Ed25519:
        return signEd25519(message, private_key);
      case Key::Type::Secp256k1:
        return CryptoProviderError::SIGNATURE_GENERATION_FAILED;
      case Key::Type::ECDSA:
        return CryptoProviderError::SIGNATURE_GENERATION_FAILED;
      case Key::Type::UNSPECIFIED:
        return KeyGeneratorError::WRONG_KEY_TYPE;
      default:
        return CryptoProviderError::SIGNATURE_GENERATION_FAILED;
    }
  }

  outcome::result<Buffer> CryptoProviderImpl::signEd25519(
      gsl::span<uint8_t> message, const PrivateKey &private_key) const {
    ed25519::PrivateKey priv_key;
    std::copy_n(private_key.data.begin(), priv_key.size(), priv_key.begin());
    OUTCOME_TRY(signature, ed25519_provider_->sign(message, priv_key));
    return {signature.begin(), signature.end()};
  }

  outcome::result<bool> CryptoProviderImpl::verify(
      gsl::span<uint8_t> message, gsl::span<uint8_t> signature,
      const PublicKey &public_key) const {
    switch (public_key.type) {
      case Key::Type::RSA:
        return CryptoProviderError::SIGNATURE_VERIFICATION_FAILED;
      case Key::Type::Ed25519:
        return verifyEd25519(message, signature, public_key);
      case Key::Type::Secp256k1:
        return CryptoProviderError::SIGNATURE_VERIFICATION_FAILED;
      case Key::Type::ECDSA:
        return CryptoProviderError::SIGNATURE_VERIFICATION_FAILED;
      case Key::Type::UNSPECIFIED:
        return KeyGeneratorError::WRONG_KEY_TYPE;
      default:
        return CryptoProviderError::SIGNATURE_VERIFICATION_FAILED;
    }
  }

  outcome::result<bool> CryptoProviderImpl::verifyEd25519(
      gsl::span<uint8_t> message, gsl::span<uint8_t> signature,
      const PublicKey &public_key) const {
    ed25519::PrivateKey ed_pub;
    std::copy_n(public_key.data.begin(), ed_pub.size(), ed_pub.begin());

    ed25519::Signature ed_sig;
    std::copy_n(signature.begin(), ed_sig.size(), ed_sig.begin());

    OUTCOME_TRY(result, ed25519_provider_->verify(message, ed_sig, ed_pub));
    return result;
  }

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
