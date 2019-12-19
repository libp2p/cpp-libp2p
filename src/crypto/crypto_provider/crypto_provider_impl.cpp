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
#include <libp2p/crypto/hmac_provider.hpp>
#include <libp2p/crypto/random_generator.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, CryptoProviderImpl::Error, e) {
  using E = libp2p::crypto::CryptoProviderImpl::Error;
  switch (e) {
    case E::UNKNOWN_CIPHER_TYPE:
      return "Unsupported cipher algorithm type was specified";
    case E::UNKNOWN_HASH_TYPE:
      return "Unsupported hash algorithm type was specified";
  }
  return "Unknown error";
}

namespace libp2p::crypto {
  CryptoProviderImpl::CryptoProviderImpl(
      std::shared_ptr<random::CSPRNG> random_provider,
      std::shared_ptr<ed25519::Ed25519Provider> ed25519_provider,
      std::shared_ptr<hmac::HmacProvider> hmac_provider)
      : random_provider_{std::move(random_provider)},
        ed25519_provider_{std::move(ed25519_provider)},
        hmac_provider_{std::move(hmac_provider)} {
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
    const auto FAILED{KeyGeneratorError::KEY_GENERATION_FAILED};
    int nid;
    size_t private_key_length_in_bytes;
    switch (curve) {
      case common::CurveType::P256:
        nid = NID_secp256k1;
        private_key_length_in_bytes = 32;
        break;
      case common::CurveType::P384:
        nid = NID_secp384r1;
        private_key_length_in_bytes = 48;
        break;
      case common::CurveType::P521:
        nid = NID_secp521r1;
        private_key_length_in_bytes = 66;  // 66 * 8 = 528 > 521
        break;
      default:
        return KeyGeneratorError::UNSUPPORTED_CURVE_TYPE;
    }

    // allocate key
    EC_KEY *key = EC_KEY_new();
    if (nullptr == key) {
      return FAILED;
    }
    auto free_private_key = gsl::finally([key] { EC_KEY_free(key); });

    // create curve
    EC_GROUP *curve_group = EC_GROUP_new_by_curve_name(nid);
    if (nullptr == curve_group) {
      return FAILED;
    }
    auto free_curve_group =
        gsl::finally([curve_group] { EC_GROUP_free(curve_group); });

    // assign curve to the key
    if (1 != EC_KEY_set_group(key, curve_group)) {
      return FAILED;
    }
    // generate the key
    if (1 != EC_KEY_generate_key(key)) {
      return FAILED;
    }

    // check that private key field is set
    const BIGNUM *private_key_bignum = EC_KEY_get0_private_key(key);
    if (nullptr == private_key_bignum) {
      return FAILED;
    }

    // serialize private key bytes and move them later to shared secret
    // generator
    std::vector<uint8_t> private_bytes(private_key_length_in_bytes, 0);
    // serialize to buffer
    if (BN_bn2binpad(private_key_bignum, private_bytes.data(),
                     private_key_length_in_bytes)
        < 0) {
      return FAILED;
    }

    // check that public key field is also set
    const EC_POINT *public_key_point = EC_KEY_get0_public_key(key);
    if (nullptr == public_key_point) {
      /*
       * Try to compute valid public key if it was not created by keygen for
       * some reason.
       *
       * This is written for safety purposes since EC_KEY_generate_key docstring
       * says the following: "Creates a new ec private (and optional a new
       * public) key."
       */
      EC_POINT *computed_public_key_point{nullptr};
      if (1
          != EC_POINT_mul(curve_group, computed_public_key_point,
                          private_key_bignum, nullptr, nullptr, nullptr)) {
        return FAILED;
      }
      if (1 != EC_KEY_set_public_key(key, computed_public_key_point)) {
        return FAILED;
      }
    }

    // marshall pubkey point to bytes buffer (uncompressed form)
    const EC_POINT *public_key{EC_KEY_get0_public_key(key)};
    uint8_t *pubkey_bytes_buffer{nullptr};
    auto free_pubkey_bytes_buffer =
        gsl::finally([pptr = &pubkey_bytes_buffer]() {
          if (*pptr != nullptr) {
            OPENSSL_free(*pptr);
          }
        });

    size_t pubkey_bytes_length{0};
    if (0
        == (pubkey_bytes_length = EC_POINT_point2buf(
                curve_group, public_key, POINT_CONVERSION_UNCOMPRESSED,
                &pubkey_bytes_buffer, nullptr))) {
      return FAILED;
    }

    auto pubkey_span = gsl::span(pubkey_bytes_buffer, pubkey_bytes_length);
    Buffer pubkey_buffer(pubkey_span.begin(), pubkey_span.end());

    return EphemeralKeyPair{
        .ephemeral_public_key = std::move(pubkey_buffer),
        .shared_secret_generator =
            prepareSharedSecretGenerator(nid, std::move(private_bytes))};
  }

  std::function<outcome::result<Buffer>(Buffer)>
  CryptoProviderImpl::prepareSharedSecretGenerator(
      int curve_nid, Buffer own_private_key) const {
    return [nid{std::move(curve_nid)},
            private_bytes{std::move(own_private_key)}](
               Buffer their_epubkey) -> outcome::result<Buffer> {
      const auto FAILED{KeyGeneratorError::KEY_DERIVATION_FAILED};
      // init curve
      EC_GROUP *curve_group = EC_GROUP_new_by_curve_name(nid);
      if (nullptr == curve_group) {
        return FAILED;
      }
      auto free_curve_group =
          gsl::finally([curve_group] { EC_GROUP_free(curve_group); });

      // create empty key
      EC_KEY *their_key = EC_KEY_new();
      if (nullptr == their_key) {
        return FAILED;
      }
      auto free_their_key =
          gsl::finally([their_key] { EC_KEY_free(their_key); });

      // assign curve type to the key
      if (1 != EC_KEY_set_group(their_key, curve_group)) {
        return FAILED;
      }

      // restore their epubkey from bytes
      const unsigned char *their_epubkey_buffer{their_epubkey.data()};
      if (nullptr
          == o2i_ECPublicKey(&their_key, &their_epubkey_buffer,
                             their_epubkey.size())) {
        return FAILED;
      }

      // get pubkey in format of point for future computations
      const EC_POINT *their_pubkey_point{EC_KEY_get0_public_key(their_key)};
      if (nullptr == their_pubkey_point) {
        return FAILED;
      }

      if (1 != EC_POINT_is_on_curve(curve_group, their_pubkey_point, nullptr)) {
        return FAILED;
      }

      // restore private key bignum from bytes
      // create new bignum for storing private key
      BIGNUM *private_bignum = BN_new();
      if (nullptr == private_bignum) {
        return FAILED;
      }
      // auto cleanup for bignum
      auto free_private_bignum =
          gsl::finally([private_bignum]() { BN_free(private_bignum); });
      // convert private key bytes to BIGNUM
      const unsigned char *private_key_buffer = private_bytes.data();
      if (nullptr
          == BN_bin2bn(private_key_buffer, private_bytes.size(),
                       private_bignum)) {
        return FAILED;
      }

      EC_KEY *our_private_key = EC_KEY_new_by_curve_name(nid);
      if (nullptr == our_private_key) {
        return FAILED;
      }
      auto free_our_private_key =
          gsl::finally([our_private_key] { EC_KEY_free(our_private_key); });

      if (1 != EC_KEY_set_private_key(our_private_key, private_bignum)) {
        return FAILED;
      }

      // calculate the size of the buffer for the shared secret
      int field_size{EC_GROUP_get_degree(curve_group)};
      int secret_len{(field_size + 7) / 8};
      uint8_t *secret_buffer{(uint8_t *)OPENSSL_malloc(secret_len)};
      if (nullptr == secret_buffer) {
        return FAILED;
      }

      secret_len =
          ECDH_compute_key(secret_buffer, secret_len, their_pubkey_point,
                           our_private_key, nullptr);
      if (secret_len <= 0) {
        OPENSSL_free(secret_buffer);
        // ^ comment to the condition above:
        // doc says it might be negative when error,
        // openssl sources show it will be zero when error
        return FAILED;
      }

      auto secret_span = gsl::span(secret_buffer, secret_len);
      Buffer secret(secret_span.begin(), secret_span.end());
      OPENSSL_free(secret_buffer);
      return secret;
    };
  }

  outcome::result<std::pair<StretchedKey, StretchedKey>>
  CryptoProviderImpl::stretchKey(common::CipherType cipher_type,
                                 common::HashType hash_type,
                                 const Buffer &secret) const {
    size_t cipher_key_size;
    size_t iv_size;
    switch (cipher_type) {
      case common::CipherType::AES128:
        cipher_key_size = 16;
        iv_size = 16;
        break;
      case common::CipherType::AES256:
        cipher_key_size = 32;
        iv_size = 16;
        break;
      default:
        return Error::UNKNOWN_CIPHER_TYPE;
    }

    constexpr size_t hmac_key_size{20};
    Buffer seed{'k', 'e', 'y', ' ', 'e', 'x', 'p',
                'a', 'n', 's', 'i', 'o', 'n'};

    size_t output_size{2 * (iv_size + cipher_key_size + hmac_key_size)};
    Buffer result;
    result.reserve(output_size);

    OUTCOME_TRY(a, hmac_provider_->calculateDigest(hash_type, secret, seed));
    while (result.size() < output_size) {
      Buffer input;
      input.reserve(a.size() + seed.size());
      input.insert(input.end(), a.begin(), a.end());
      input.insert(input.end(), seed.begin(), seed.end());

      OUTCOME_TRY(b, hmac_provider_->calculateDigest(hash_type, secret, input));
      size_t todo = b.size();
      if (result.size() + todo > output_size) {
        todo = output_size - result.size();
      }
      std::copy_n(b.begin(), todo, std::back_inserter(result));
      OUTCOME_TRY(c, hmac_provider_->calculateDigest(hash_type, secret, a));
      a = std::move(c);
    }

    auto iter = result.begin();
    Buffer k1_iv{iter, iter += iv_size};
    Buffer k1_cipher_key{iter, iter += cipher_key_size};
    Buffer k1_mac_key{iter, iter += hmac_key_size};

    Buffer k2_iv{iter, iter += iv_size};
    Buffer k2_cipher_key{iter, iter += cipher_key_size};
    Buffer k2_mac_key{iter, iter += hmac_key_size};

    return std::make_pair<StretchedKey, StretchedKey>(
        {.iv = std::move(k1_iv),
         .cipher_key = std::move(k1_cipher_key),
         .mac_key = std::move(k1_mac_key)},
        {.iv = std::move(k2_iv),
         .cipher_key = std::move(k2_cipher_key),
         .mac_key = std::move(k2_mac_key)});
  }

}  // namespace libp2p::crypto
