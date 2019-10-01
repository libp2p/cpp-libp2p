/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/key_generator/key_generator_impl.hpp>

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
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/random_generator.hpp>

namespace libp2p::crypto {
  KeyGeneratorImpl::KeyGeneratorImpl(random::CSPRNG &random_provider)
      : random_provider_(random_provider) {
    initialize();
  }

  void KeyGeneratorImpl::initialize() {
    constexpr size_t kSeedBytesCount = 128 * 4;  // ripple uses such number
    auto bytes = random_provider_.randomBytes(kSeedBytesCount);
    // seeding random generator is required prior to calling RSA_generate_key
    // NOLINTNEXTLINE
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
      BIO *bio_private = BIO_new_mem_buf(
          static_cast<const void *>(key.data.data()), key.data.size());
      if (nullptr == bio_private) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      auto free_bio =
          gsl::finally([bio_private]() { BIO_free_all(bio_private); });

      const unsigned char *data_pointer = key.data.data();
      RSA *rsa = d2i_RSAPrivateKey(nullptr, &data_pointer, key.data.size());
      if (nullptr == rsa) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      auto cleanup_rsa = gsl::finally([rsa]() { RSA_free(rsa); });

      OUTCOME_TRY(public_bytes, detail::encodeKeyDer(rsa, i2d_RSAPublicKey));

      return PublicKey{{key.type, std::move(public_bytes)}};
    }

    outcome::result<PublicKey> deriveEd25519(const PrivateKey &key) {
      EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(
          EVP_PKEY_ED25519, nullptr, key.data.data(), key.data.size());
      if (nullptr == pkey) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      auto free_pkey = gsl::finally([pkey] { EVP_PKEY_free(pkey); });

      OUTCOME_TRY(public_buffer,
                  getEvpPkeyRawBytes(pkey, EVP_PKEY_get_raw_public_key));

      return PublicKey{{key.type, std::move(public_buffer)}};
    }

    outcome::result<PublicKey> deriveSecp256k1(const PrivateKey &key) {
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

      // generate public key from private using bitcoin curve

      // get bitcoin curve group
      EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);
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
      if (public_length < 0) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      auto span = gsl::span(data_pointer, public_length);
      std::vector<uint8_t> public_buffer(span.begin(), span.end());

      return PublicKey{{key.type, std::move(public_buffer)}};
    }

    /**
     * according to libp2p specification:
     *  https://github.com/libp2p/specs/blob/master/peer-ids/peer-ids.md#how-keys-are-encoded-and-messages-signed
     *  We encode the public key using the DER-encoded PKIX format
     *  We encode the private key as a PKCS1 key using ASN.1 DER.
     *
     *  according to openssl manual:
     *  https://www.openssl.org/docs/man1.0.2/man3/i2d_RSAPublicKey.html
     *  d2i_RSAPublicKey() and i2d_RSAPublicKey() decode and encode a PKCS#1
     *
     *  https://www.openssl.org/docs/man1.0.2/man3/i2d_RSAPrivateKey.html
     *  d2i_RSAPrivateKey(), i2d_RSAPrivateKey() decode and encode a PKCS#1
     */
    outcome::result<std::pair<KeyGenerator::Buffer, KeyGenerator::Buffer>>
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

      OUTCOME_TRY(public_bytes, detail::encodeKeyDer(rsa, i2d_RSAPublicKey));
      OUTCOME_TRY(private_bytes, detail::encodeKeyDer(rsa, i2d_RSAPrivateKey));

      return {std::move(public_bytes), std::move(private_bytes)};
    }
  }  // namespace detail

  outcome::result<KeyPair> KeyGeneratorImpl::generateKeys(
      Key::Type key_type) const {
    switch (key_type) {
      case Key::Type::RSA:
        BOOST_ASSERT_MSG(false, "not implemented");
      case Key::Type::Ed25519:
        return generateEd25519();
      case Key::Type::Secp256k1:
        return generateSecp256k1();
      default:
        return KeyGeneratorError::UNSUPPORTED_KEY_TYPE;
    }
  }

  outcome::result<KeyPair> KeyGeneratorImpl::generateRsa(
      common::RSAKeyType bits_option) const {
    throw std::logic_error{"Not implemented"};

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
  }

  outcome::result<KeyPair> KeyGeneratorImpl::generateEd25519() const {
    EVP_PKEY *pkey = nullptr;
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);

    auto cleanup = gsl::finally([&]() {
      if (pkey != nullptr) {
        EVP_PKEY_free(pkey);
      }
      if (pctx != nullptr) {
        EVP_PKEY_CTX_free(pctx);
      }
    });

    auto ret = EVP_PKEY_keygen_init(pctx);
    if (ret != 1) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    ret = EVP_PKEY_keygen(pctx, &pkey);
    if (ret != 1) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    OUTCOME_TRY(private_key_bytes,
                detail::getEvpPkeyRawBytes(pkey, EVP_PKEY_get_raw_private_key));
    OUTCOME_TRY(public_key_bytes,
                detail::getEvpPkeyRawBytes(pkey, EVP_PKEY_get_raw_public_key));

    return KeyPair{{{Key::Type::Ed25519, std::move(public_key_bytes)}},
                   {{Key::Type::Ed25519, std::move(private_key_bytes)}}};
  }

  outcome::result<KeyPair> KeyGeneratorImpl::generateSecp256k1() const {
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

    // get bitcoin curve group
    EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);
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

    return KeyPair{{{Key::Type::Secp256k1, std::move(public_bytes)}},
                   {{Key::Type::Secp256k1, std::move(private_bytes)}}};
  }

  outcome::result<PublicKey> KeyGeneratorImpl::derivePublicKey(
      const PrivateKey &private_key) const {
    switch (private_key.type) {
      case Key::Type::RSA:
        return detail::deriveRsa(private_key);
      case Key::Type::Ed25519:
        return detail::deriveEd25519(private_key);
      case Key::Type::Secp256k1:
        return detail::deriveSecp256k1(private_key);
      case Key::Type::UNSPECIFIED:
        return KeyGeneratorError::WRONG_KEY_TYPE;
    }
    return KeyGeneratorError::UNSUPPORTED_KEY_TYPE;
  }

  outcome::result<EphemeralKeyPair> KeyGeneratorImpl::generateEphemeralKeyPair(
      common::CurveType curve) const {
    // TODO(yuraz): pre-140 implement
    return KeyGeneratorError::KEY_GENERATION_FAILED;
  }

  std::vector<StretchedKey> KeyGeneratorImpl::stretchKey(
      common::CipherType cipher_type, common::HashType hash_type,
      const Buffer &secret) const {
    // TODO(yuraz): pre-140 implement
    return {};
  }
}  // namespace libp2p::crypto
