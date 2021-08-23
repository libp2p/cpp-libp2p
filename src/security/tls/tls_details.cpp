/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openssl/asn1.h>
#include <openssl/x509_vfy.h>
#include <boost/asio/ssl.hpp>
#include <boost/optional.hpp>

#include <libp2p/crypto/ecdsa_provider/ecdsa_provider_impl.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/security/tls/tls_errors.hpp>

#include "tls_details.hpp"

namespace libp2p::security::tls_details {

  namespace {

    // Helper useful for auto-cleanup of OpenSSL object ptrs
    template <class Obj>
    struct Cleanup {  // NOLINT
      using Deleter = void(Obj *);

      Obj *obj;
      Deleter *deleter;

      Cleanup(Obj *o, Deleter *d) : obj(o), deleter(d) {}

      ~Cleanup() {
        deleter(obj);
      }
    };

#define COMBINE0(X, Y) X##Y
#define COMBINE(X, Y) COMBINE0(X, Y)
#define CLEANUP_PTR(P, F) Cleanup COMBINE(cleanup, __LINE__){P, F};

  }  // namespace

  log::Logger log() {
    static log::Logger logger = log::createLogger("TLS");
    return logger;
  }

  static const char *x509ErrorToStr(int error);

  bool verifyCallback(bool status, boost::asio::ssl::verify_context &ctx) {
    X509_STORE_CTX *store_ctx = ctx.native_handle();
    assert(store_ctx);

    int error = X509_STORE_CTX_get_error(store_ctx);
    int32_t depth = X509_STORE_CTX_get_error_depth(store_ctx);

    if (error == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN
        || error == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) {
      // we make and accept self-signed certs for libp2p
      status = true;
    }

    X509 *cert = X509_STORE_CTX_get_current_cert(store_ctx);
    if (cert == nullptr) {
      return false;
    }

    std::array<char, 256> subject_name{};
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name.data(),
                      subject_name.size());

    log()->log(status ? log::Level::TRACE : log::Level::INFO,
              "in certificate verify callback, subject={}, error={} ({}), "
              "depth={}, status={}",
              subject_name.data(), x509ErrorToStr(error), error, depth, status);
    return status;
  }

  namespace {

    // libp2p-specific extension's OID
    constexpr std::string_view extension_oid = "1.3.6.1.4.1.53594.1.1";

    // prefix for extension's message signature
    constexpr std::string_view sign_prefix = "libp2p-tls-handshake:";

    // ASN1 tags
    constexpr uint8_t kSequenceTag = 0x30;
    constexpr uint8_t kOctetStringTag = 0x04;

    // Size constants of the extension's format
    constexpr size_t kMarshalledPublicKeySize = 36;
    constexpr size_t kSignatureSize = 64;
    constexpr size_t kAsnHeaderSize = 2;
    constexpr size_t kSigOffset = 2 * kAsnHeaderSize + kMarshalledPublicKeySize;
    constexpr size_t kExtensionDataSize =
        kSigOffset + kAsnHeaderSize + kSignatureSize;

    using Blob = std::vector<uint8_t>;
    using Signature = std::array<uint8_t, kSignatureSize>;

    // packs extension data into ASN1 sequence of 2 octet strings
    std::array<uint8_t, kExtensionDataSize> marshalExtensionData(
        const Blob &pub_key_bytes, const Signature &signature) {
      if (pub_key_bytes.size() != kMarshalledPublicKeySize) {
        throw std::runtime_error(
            "unexpected size of peer's marshalled public key");
      }

      std::array<uint8_t, kExtensionDataSize> result{};

      result[0] = kSequenceTag;

      // data length
      result[1] = kExtensionDataSize - kAsnHeaderSize;

      result[2] = kOctetStringTag;
      result[3] = kMarshalledPublicKeySize;
      memcpy(&result[4], pub_key_bytes.data(), kMarshalledPublicKeySize);

      result[kSigOffset] = kOctetStringTag;
      result[kSigOffset + 1] = kSignatureSize;
      memcpy(&result[kSigOffset + 2], signature.data(), kSignatureSize);

      return result;
    }

    // make signature for the extension which ties certificate and host keys
    Signature makeExtensionSignature(
        const crypto::ecdsa::PublicKey &cert_pub_key,
        const crypto::PrivateKey &host_private_key) {
      constexpr size_t prefix_size = sign_prefix.size();
      size_t msg_len = prefix_size + cert_pub_key.size();

      uint8_t buf[msg_len];  // NOLINT
      memcpy(buf, sign_prefix.data(), sign_prefix.size());
      memcpy(buf + sign_prefix.size(),  // NOLINT
             cert_pub_key.data(), cert_pub_key.size());

      assert(host_private_key.type == crypto::Key::Type::Ed25519);
      assert(host_private_key.data.size() == 32);

      std::array<uint8_t, 32> pk_data{};
      memcpy(pk_data.data(), host_private_key.data.data(), 32);

      return crypto::ed25519::Ed25519ProviderImpl{}
          .sign(gsl::span<const uint8_t>(buf, msg_len), pk_data)
          .value();
    }

    // set certificate's public key
    void assignPubkey(X509 *cert,
                      const crypto::ecdsa::PublicKey &cert_pub_key) {
      EVP_PKEY *pub = nullptr;
      const auto *data = cert_pub_key.data();
      d2i_PUBKEY(&pub, &data, cert_pub_key.size());
      if (pub == nullptr) {
        throw std::runtime_error("cannot deserialize certificate public key");
      }
      CLEANUP_PTR(pub, EVP_PKEY_free);
      if (X509_set_pubkey(cert, pub) == 0) {
        throw std::runtime_error("cannot add public key to certificate");
      }
    }

    // assigns random SN to certificate
    void assignSerial(X509 *cert) {
      BIGNUM *bn = BN_new();
      CLEANUP_PTR(bn, BN_free);
      if (BN_pseudo_rand(bn, 64, 0, 0) == 0) {
        throw std::runtime_error("BN_pseudo_rand failed");
      }

      ASN1_INTEGER *rand_int = ASN1_INTEGER_new();
      BN_to_ASN1_INTEGER(bn, rand_int);

      if (X509_set_serialNumber(cert, rand_int) == 0) {
        ASN1_INTEGER_free(rand_int);
        throw std::runtime_error("cannot add SN to certificate");
      }
    }

    void assignIssuer(X509 *cert) {
      X509_gmtime_adj(X509_get_notBefore(cert), 0);
      X509_gmtime_adj(X509_get_notAfter(cert), 10LL * 60 * 60 * 24 * 365);
      X509_NAME *name = X509_get_subject_name(cert);
      if (name == nullptr) {
        throw std::runtime_error("cannot get certificate subject name");
      }
      X509_NAME_add_entry_by_txt(
          name, "C", MBSTRING_ASC,
          reinterpret_cast<const uint8_t *>("PY"),  // NOLINT
          -1, -1, 0);
      X509_NAME_add_entry_by_txt(
          name, "O", MBSTRING_ASC,
          reinterpret_cast<const uint8_t *>("libp2p"),  // NOLINT
          -1, -1, 0);
      X509_NAME_add_entry_by_txt(
          name, "CN", MBSTRING_ASC,
          reinterpret_cast<const uint8_t *>("libp2p"),  // NOLINT
          -1, -1, 0);
      if (X509_set_issuer_name(cert, name) == 0) {
        throw std::runtime_error("cannot assign certificate issuer name");
      }
    }

    void insertExtension(
        X509 *cert, const std::array<uint8_t, kExtensionDataSize> &ext_data) {
      ASN1_OCTET_STRING *os = ASN1_OCTET_STRING_new();
      CLEANUP_PTR(os, ASN1_OCTET_STRING_free);
      ASN1_OCTET_STRING_set(os, ext_data.data(), ext_data.size());

      ASN1_OBJECT *obj = OBJ_txt2obj(extension_oid.data(), 1);
      CLEANUP_PTR(obj, ASN1_OBJECT_free);

      X509_EXTENSION *ex = X509_EXTENSION_create_by_OBJ(nullptr, obj, 0, os);
      if (ex == nullptr) {
        throw std::runtime_error("cannot create extension");
      }
      X509_add_ext(cert, ex, -1);
      X509_EXTENSION_free(ex);
    }

    void signCertificate(X509 *cert,
                         const crypto::ecdsa::PrivateKey &priv_key) {
      EC_KEY *ec_key = nullptr;
      const uint8_t *data = priv_key.data();
      d2i_ECPrivateKey(&ec_key, &data, priv_key.size());
      if (ec_key == nullptr) {
        throw std::runtime_error("cannot deserialize ECDSA private key");
      }
      EVP_PKEY *evp_key = EVP_PKEY_new();
      CLEANUP_PTR(evp_key, EVP_PKEY_free);
      if (EVP_PKEY_assign_EC_KEY(evp_key, ec_key) == 0) {  // NOLINT
        throw std::runtime_error("cannot assign private key to EVP_PKEY");
      }
      if (X509_sign(cert, evp_key, EVP_sha256()) == 0) {
        throw std::runtime_error("cannot sign certificate");
      }
    }

  }  // namespace

  CertificateAndKey makeCertificate(
      const crypto::KeyPair &host_key_pair,
      const crypto::marshaller::KeyMarshaller &key_marshaller) {
    CertificateAndKey ret;

    // 1. Generate ECDSA keypair for certificate / ssl context
    auto cert_keys = crypto::ecdsa::EcdsaProviderImpl{}.generate().value();
    ret.private_key = cert_keys.private_key;

    // 2. Make extension data
    auto pub_key_bytes =
        key_marshaller.marshal(host_key_pair.publicKey).value().key;
    auto signature =
        makeExtensionSignature(cert_keys.public_key, host_key_pair.privateKey);
    auto extension_data = marshalExtensionData(pub_key_bytes, signature);

    // 3. Create certificate
    X509 *cert = X509_new();
    CLEANUP_PTR(cert, X509_free);
    X509_set_version(cert, 3);
    assignPubkey(cert, cert_keys.public_key);
    assignSerial(cert);
    assignIssuer(cert);
    insertExtension(cert, extension_data);
    signCertificate(cert, cert_keys.private_key);

    // 4. Serialize into ASN1 DER
    int size = i2d_X509(cert, nullptr);
    ret.certificate.resize(size);
    unsigned char *cert_data = ret.certificate.data();
    i2d_X509(cert, &cert_data);

    return ret;
  }

  namespace {

    struct KeyAndSignature {
      Blob pkey;
      std::array<uint8_t, kSignatureSize> signature{};
    };

    // extracts peer's pubkey and extension signature from ASN1 sequence
    boost::optional<KeyAndSignature> unmarshalExtensionData(
        gsl::span<const uint8_t> data) {
      KeyAndSignature result;

      bool ok = (data.size() == kExtensionDataSize) && (data[0] == kSequenceTag)
          && (data[1] == kExtensionDataSize - kAsnHeaderSize)
          && (data[2] == kOctetStringTag)
          && (data[3] == kMarshalledPublicKeySize)
          && (data[kSigOffset] == kOctetStringTag)
          && (data[kSigOffset + 1] == kSignatureSize);

      if (!ok) {
        return boost::none;
      }

      auto slice = data.subspan(4, kMarshalledPublicKeySize);
      result.pkey.assign(slice.begin(), slice.end());
      memcpy(result.signature.data(), &data[kSigOffset + kAsnHeaderSize],
             kSignatureSize);

      return result;
    }

    // extracts extension fields
    outcome::result<KeyAndSignature> extractExtensionFields(
        X509 *peer_certificate) {
      ASN1_OBJECT *obj = OBJ_txt2obj(extension_oid.data(), 1);
      CLEANUP_PTR(obj, ASN1_OBJECT_free);

      int index = X509_get_ext_by_OBJ(peer_certificate, obj, -1);
      if (index < 0) {
        log()->info("cannot find libp2p certificate extension");
        return TlsError::TLS_INCOMPATIBLE_CERTIFICATE_EXTENSION;
      }

      X509_EXTENSION *ext = X509_get_ext(peer_certificate, index);
      assert(ext);

      ASN1_OCTET_STRING *os = X509_EXTENSION_get_data(ext);
      assert(os);

      auto ks_binary = unmarshalExtensionData(
          gsl::span<const uint8_t>(os->data,
                                   os->data + os->length));  // NOLINT
      if (!ks_binary) {
        log()->info("cannot unmarshal libp2p certificate extension");
        return TlsError::TLS_INCOMPATIBLE_CERTIFICATE_EXTENSION;
      }

      return std::move(ks_binary.value());
    }

    outcome::result<void> verifyExtensionSignature(
        X509 *peer_certificate, const crypto::PublicKey &peer_pubkey,
        const Signature &signature, const peer::PeerId &peer_id) {
      crypto::ed25519::PublicKey ed25519pkey;
      assert(peer_pubkey.data.size() == ed25519pkey.size());

      memcpy(ed25519pkey.data(), peer_pubkey.data.data(), ed25519pkey.size());

      EVP_PKEY *cert_pubkey = X509_get_pubkey(peer_certificate);
      assert(cert_pubkey);
      int len = i2d_PUBKEY(cert_pubkey, nullptr);
      assert(len == 91);

      constexpr size_t prefix_size = sign_prefix.size();

      size_t msg_len = prefix_size + len;
      uint8_t buf[msg_len];  // NOLINT
      memcpy(buf, sign_prefix.data(), prefix_size);
      uint8_t *b = buf + prefix_size;  // NOLINT
      i2d_PUBKEY(cert_pubkey, &b);

      auto verify_res = crypto::ed25519::Ed25519ProviderImpl{}.verify(
          gsl::span<const uint8_t>(buf, buf + msg_len),  // NOLINT
          signature, ed25519pkey);

      if (!verify_res) {
        log()->info("peer {} verification failed, {}", peer_id.toBase58(),
                   verify_res.error().message());
        return TlsError::TLS_PEER_VERIFY_FAILED;
      }

      if (!verify_res.value()) {
        log()->info("peer {} verification failed", peer_id.toBase58());
        return TlsError::TLS_PEER_VERIFY_FAILED;
      }

      return outcome::success();
    }

  }  // namespace

  outcome::result<PubkeyAndPeerId> verifyPeerAndExtractIdentity(
      X509 *peer_certificate,
      const crypto::marshaller::KeyMarshaller &key_marshaller) {
    // 1. Extract fields from cert extension
    OUTCOME_TRY(bin_fields, extractExtensionFields(peer_certificate));

    // 2. Try to extract peer id and pubkey from protobuf format
    crypto::ProtobufKey pub_key_bytes(std::move(bin_fields.pkey));

    auto peer_id_res = peer::PeerId::fromPublicKey(pub_key_bytes);
    if (!peer_id_res) {
      log()->info("cannot unmarshal remote peer id");
      return TlsError::TLS_INCOMPATIBLE_CERTIFICATE_EXTENSION;
    }

    auto peer_pubkey_res = key_marshaller.unmarshalPublicKey(pub_key_bytes);
    if (!peer_pubkey_res) {
      log()->info("cannot unmarshal remote public key");
      return TlsError::TLS_INCOMPATIBLE_CERTIFICATE_EXTENSION;
    }

    if (peer_pubkey_res.value().type != crypto::Key::Type::Ed25519) {
      log()->info("remote peer's public key wrong type");
      return TlsError::TLS_INCOMPATIBLE_CERTIFICATE_EXTENSION;
    }

    // 3. Verify
    OUTCOME_TRY(
        verifyExtensionSignature(peer_certificate, peer_pubkey_res.value(),
                                 bin_fields.signature, peer_id_res.value()));

    return PubkeyAndPeerId{std::move(peer_pubkey_res.value()),
                           std::move(peer_id_res.value())};
  }

  const char *x509ErrorToStr(int error) {
    switch (error) {
#define CASE_X(E) \
  case E:         \
    return #E;

      CASE_X(X509_V_OK)
      CASE_X(X509_V_ERR_UNSPECIFIED)
      CASE_X(X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)
      CASE_X(X509_V_ERR_UNABLE_TO_GET_CRL)
      CASE_X(X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE)
      CASE_X(X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE)
      CASE_X(X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY)
      CASE_X(X509_V_ERR_CERT_SIGNATURE_FAILURE)
      CASE_X(X509_V_ERR_CRL_SIGNATURE_FAILURE)
      CASE_X(X509_V_ERR_CERT_NOT_YET_VALID)
      CASE_X(X509_V_ERR_CERT_HAS_EXPIRED)
      CASE_X(X509_V_ERR_CRL_NOT_YET_VALID)
      CASE_X(X509_V_ERR_CRL_HAS_EXPIRED)
      CASE_X(X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD)
      CASE_X(X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD)
      CASE_X(X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD)
      CASE_X(X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD)
      CASE_X(X509_V_ERR_OUT_OF_MEM)
      CASE_X(X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
      CASE_X(X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
      CASE_X(X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)
      CASE_X(X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE)
      CASE_X(X509_V_ERR_CERT_CHAIN_TOO_LONG)
      CASE_X(X509_V_ERR_CERT_REVOKED)
      CASE_X(X509_V_ERR_INVALID_CA)
      CASE_X(X509_V_ERR_PATH_LENGTH_EXCEEDED)
      CASE_X(X509_V_ERR_INVALID_PURPOSE)
      CASE_X(X509_V_ERR_CERT_UNTRUSTED)
      CASE_X(X509_V_ERR_CERT_REJECTED)
      CASE_X(X509_V_ERR_SUBJECT_ISSUER_MISMATCH)
      CASE_X(X509_V_ERR_AKID_SKID_MISMATCH)
      CASE_X(X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH)
      CASE_X(X509_V_ERR_KEYUSAGE_NO_CERTSIGN)
      CASE_X(X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER)
      CASE_X(X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION)
      CASE_X(X509_V_ERR_KEYUSAGE_NO_CRL_SIGN)
      CASE_X(X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION)
      CASE_X(X509_V_ERR_INVALID_NON_CA)
      CASE_X(X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED)
      CASE_X(X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE)
      CASE_X(X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED)
      CASE_X(X509_V_ERR_INVALID_EXTENSION)
      CASE_X(X509_V_ERR_INVALID_POLICY_EXTENSION)
      CASE_X(X509_V_ERR_NO_EXPLICIT_POLICY)
      CASE_X(X509_V_ERR_DIFFERENT_CRL_SCOPE)
      CASE_X(X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE)
      CASE_X(X509_V_ERR_UNNESTED_RESOURCE)
      CASE_X(X509_V_ERR_PERMITTED_VIOLATION)
      CASE_X(X509_V_ERR_EXCLUDED_VIOLATION)
      CASE_X(X509_V_ERR_SUBTREE_MINMAX)
      CASE_X(X509_V_ERR_APPLICATION_VERIFICATION)
      CASE_X(X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE)
      CASE_X(X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX)
      CASE_X(X509_V_ERR_UNSUPPORTED_NAME_SYNTAX)
      CASE_X(X509_V_ERR_CRL_PATH_VALIDATION_ERROR)
      CASE_X(X509_V_ERR_PATH_LOOP)
      CASE_X(X509_V_ERR_SUITE_B_INVALID_VERSION)
      CASE_X(X509_V_ERR_SUITE_B_INVALID_ALGORITHM)
      CASE_X(X509_V_ERR_SUITE_B_INVALID_CURVE)
      CASE_X(X509_V_ERR_SUITE_B_INVALID_SIGNATURE_ALGORITHM)
      CASE_X(X509_V_ERR_SUITE_B_LOS_NOT_ALLOWED)
      CASE_X(X509_V_ERR_SUITE_B_CANNOT_SIGN_P_384_WITH_P_256)
      CASE_X(X509_V_ERR_HOSTNAME_MISMATCH)
      CASE_X(X509_V_ERR_EMAIL_MISMATCH)
      CASE_X(X509_V_ERR_IP_ADDRESS_MISMATCH)
      CASE_X(X509_V_ERR_DANE_NO_MATCH)
      CASE_X(X509_V_ERR_EE_KEY_TOO_SMALL)
      CASE_X(X509_V_ERR_CA_KEY_TOO_SMALL)
      CASE_X(X509_V_ERR_CA_MD_TOO_WEAK)
      CASE_X(X509_V_ERR_INVALID_CALL)
      CASE_X(X509_V_ERR_STORE_LOOKUP)
      CASE_X(X509_V_ERR_NO_VALID_SCTS)
      CASE_X(X509_V_ERR_PROXY_SUBJECT_NAME_VIOLATION)
      CASE_X(X509_V_ERR_OCSP_VERIFY_NEEDED)
      CASE_X(X509_V_ERR_OCSP_VERIFY_FAILED)
      CASE_X(X509_V_ERR_OCSP_CERT_UNKNOWN)

#undef CASE_X
      default:
        break;
    }
    return "unknown x509 error";
  }

}  // namespace libp2p::security::tls_details

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security, TlsError, e) {
  using E = libp2p::security::TlsError;
  switch (e) {
    case E::TLS_CTX_INIT_FAILED:
      return "Cannot initialize SSL context";
    case E::TLS_INCOMPATIBLE_TRANSPORT:
      return "Incompatible underlying transport";
    case E::TLS_NO_CERTIFICATE:
      return "No peer certificate";
    case E::TLS_INCOMPATIBLE_CERTIFICATE_EXTENSION:
      return "Incompatible TLS certificate extension";
    case E::TLS_PEER_VERIFY_FAILED:
      return "Peer verify failed";
    case E::TLS_UNEXPECTED_PEER_ID:
      return "Unexpected remote peer id";
    case E::TLS_REMOTE_PEER_NOT_AVAILABLE:
      return "Remote peer not available";
    case E::TLS_REMOTE_PUBKEY_NOT_AVAILABLE:
      return "Remote public key not available";
    default:
      break;
  }
  return "Tls: Unknown error";
}
