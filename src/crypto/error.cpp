/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/error.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, CryptoProviderError, e) {
  using libp2p::crypto::CryptoProviderError;
  switch (e) {
    case CryptoProviderError::INVALID_KEY_TYPE:
      return "failed to unmarshal key type, invalid value";
    case CryptoProviderError::UNKNOWN_KEY_TYPE:
      return "failed to unmarshal key";
    case CryptoProviderError::FAILED_UNMARSHAL_DATA:
      return "google protobuf error, failed to unmarshal data";
    case CryptoProviderError::SIGNATURE_GENERATION_FAILED:
      return "error while signing data";
    case CryptoProviderError::SIGNATURE_VERIFICATION_FAILED:
      return "error while verifying signature";
  }
  return "unknown CryptoProviderError code";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, OpenSslError, e) {
  using libp2p::crypto::OpenSslError;
  switch (e) {
    case OpenSslError::FAILED_INITIALIZE_CONTEXT:
      return "failed to initialize context";
    case OpenSslError::FAILED_INITIALIZE_OPERATION:
      return "failed to initialize operation";
    case OpenSslError::FAILED_ENCRYPT_UPDATE:
      return "failed to update encryption";
    case OpenSslError::FAILED_DECRYPT_UPDATE:
      return "failed to update decryption";
    case OpenSslError::FAILED_ENCRYPT_FINALIZE:
      return "failed to finalize encryption";
    case OpenSslError::FAILED_DECRYPT_FINALIZE:
      return "failed to finalize decryption";
    case OpenSslError::WRONG_IV_SIZE:
      return "wrong iv size";
    case OpenSslError::WRONG_KEY_SIZE:
      return "wrong key size";
    case OpenSslError::STREAM_FINALIZED:
      return "stream encryption(decryption) has been already finalized";
  }
  return "unknown CryptoProviderError code";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, HmacProviderError, e) {
  using libp2p::crypto::HmacProviderError;
  switch (e) {
    case HmacProviderError::UNSUPPORTED_HASH_METHOD:
      return "hash method id provided is not supported";
    case HmacProviderError::FAILED_CREATE_CONTEXT:
      return "failed to create context";
    case HmacProviderError::FAILED_INITIALIZE_CONTEXT:
      return "failed to initialize context";
    case HmacProviderError::FAILED_UPDATE_DIGEST:
      return "failed to update digest";
    case HmacProviderError::FAILED_FINALIZE_DIGEST:
      return "failed to finalize digest";
    case HmacProviderError::WRONG_DIGEST_SIZE:
      return "wrong digest size";
  }
  return "unknown HmacProviderError code";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, RandomProviderError, e) {
  using libp2p::crypto::RandomProviderError;
  switch (e) {
    case RandomProviderError::FAILED_OPEN_FILE:
      return "failed to open file at specified path";
    case RandomProviderError::TOKEN_NOT_EXISTS:
      return "specified random source doesn't exist in system";
    case RandomProviderError::FAILED_FETCH_BYTES:
      return "failed to fetch bytes from source";
    case RandomProviderError::INVALID_PROVIDER_TYPE:
      return "invalid or unsupported random provider type";
  }
  return "unknown RandomProviderError code";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, KeyGeneratorError, e) {
  using libp2p::crypto::KeyGeneratorError;
  switch (e) {
    case KeyGeneratorError::CANNOT_GENERATE_UNSPECIFIED:
      return "you need to specify valid key type";
    case KeyGeneratorError::UNKNOWN_KEY_TYPE:
      return "unknown key type";
    case KeyGeneratorError::GENERATOR_NOT_INITIALIZED:
      return "generator not initialized";
    case KeyGeneratorError::KEY_GENERATION_FAILED:
      return "key generation failed";
    case KeyGeneratorError::KEY_DERIVATION_FAILED:
      return "failed to derive key";
    case KeyGeneratorError::FILE_NOT_FOUND:
      return "file not found";
    case KeyGeneratorError::FAILED_TO_READ_FILE:
      return "failed to read file";
    case KeyGeneratorError::INCORRECT_BITS_COUNT:
      return "incorrect bits count";
    case KeyGeneratorError::WRONG_KEY_TYPE:
      return "incorrect key type";
    case KeyGeneratorError::UNSUPPORTED_KEY_TYPE:
      return "key type is not supported";
    case KeyGeneratorError::CANNOT_LOAD_UNSPECIFIED:
      return "cannot load unspecified key";
    case KeyGeneratorError::GET_KEY_BYTES_FAILED:
      return "failed to get key bytes from PKEY";
    case KeyGeneratorError::INTERNAL_ERROR:
      return "internal error happened";
    case KeyGeneratorError::UNSUPPORTED_CURVE_TYPE:
      return "cannot init requested curve for key generation";
  }
  return "unknown key generation error";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, KeyValidatorError, e) {
  using libp2p::crypto::KeyValidatorError;
  switch (e) {
    case KeyValidatorError::WRONG_PUBLIC_KEY_SIZE:
      return "public key has wrong size";
    case KeyValidatorError::WRONG_PRIVATE_KEY_SIZE:
      return "private key has wrong size";
    case KeyValidatorError::INVALID_PUBLIC_KEY:
      return "public key cannot be decoded";
    case KeyValidatorError::INVALID_PRIVATE_KEY:
      return "private key cannot be decoded";
    case KeyValidatorError::KEYS_DONT_MATCH:
      return "public key is not derived from private";
    case KeyValidatorError::DIFFERENT_KEY_TYPES:
      return "key types in pair don't match";
    case KeyValidatorError::UNSUPPORTED_CRYPTO_ALGORYTHM:
      return "crypto algorythm is not implemented";
    case KeyValidatorError::UNKNOWN_CRYPTO_ALGORYTHM:
      return "unknown crypto algorythm";
  }

  return "unknown KeyValidator error";
}
