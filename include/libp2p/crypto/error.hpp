/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_ERROR_HPP
#define LIBP2P_CRYPTO_ERROR_HPP

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto {
  enum class CryptoProviderError {
    INVALID_KEY_TYPE = 1,         ///< failed to unmarshal key type, wrong value
    UNKNOWN_KEY_TYPE,             ///< failed to unmarshal key
    FAILED_UNMARSHAL_DATA,        ///< protobuf error, failed to unmarshal data
    SIGNATURE_GENERATION_FAILED,  ///< general signature creation failure
    SIGNATURE_VERIFICATION_FAILED,  ///< general signature verification failure
  };

  enum class OpenSslError {
    FAILED_INITIALIZE_CONTEXT = 1,  ///< failed to initialize context
    FAILED_INITIALIZE_OPERATION,    ///< failed to initialize operation
    FAILED_ENCRYPT_UPDATE,          ///< failed to update encryption
    FAILED_DECRYPT_UPDATE,          ///< failed to update decryption
    FAILED_ENCRYPT_FINALIZE,        ///< failed to finalize encryption
    FAILED_DECRYPT_FINALIZE,        ///< failed to finalize decryption
    WRONG_IV_SIZE,                  ///< wrong iv size
    WRONG_KEY_SIZE,                 ///< wrong key size
    STREAM_FINALIZED,  ///< crypt update operations cannot be performed after
                       ///< stream finalization
  };

  enum class HmacProviderError {
    UNSUPPORTED_HASH_METHOD = 1,  ///< hash method id provided is not supported
    FAILED_CREATE_CONTEXT,        ///< failed to create context
    FAILED_INITIALIZE_CONTEXT,    ///< failed to initialize context
    FAILED_UPDATE_DIGEST,         ///< failed to update digest
    FAILED_FINALIZE_DIGEST,       ///< failed to finalize digest
    WRONG_DIGEST_SIZE,            ///< wrong digest size
  };

  enum class RandomProviderError {
    TOKEN_NOT_EXISTS = 1,   ///< /dev/urandom path not present in system
    FAILED_OPEN_FILE,       ///< failed to open random provider file
    FAILED_FETCH_BYTES,     ///< failed to fetch bytes from source
    INVALID_PROVIDER_TYPE,  ///< invalid or unsupported random provider type
  };

  enum class KeyGeneratorError {
    CANNOT_GENERATE_UNSPECIFIED = 1,  ///< you need to specify valid key type
    UNKNOWN_KEY_TYPE,                 ///< unknown key type
    GENERATOR_NOT_INITIALIZED,        ///< generator not initialized
    KEY_GENERATION_FAILED,            ///< key generation failed
    KEY_DERIVATION_FAILED,            ///< failed to derive key
    FILE_NOT_FOUND,                   ///< file not found
    FAILED_TO_READ_FILE,              ///< failed to read file
    INCORRECT_BITS_COUNT,             ///< incorrect bits option
    UNSUPPORTED_KEY_TYPE,             ///< key type is not supported
    WRONG_KEY_TYPE,                   ///< incorrect key type
    CANNOT_LOAD_UNSPECIFIED,          ///< cannot load unspecified key
    GET_KEY_BYTES_FAILED,             ///< failed to get key bytes from PKEY
    INTERNAL_ERROR,                   ///< internal error happened
    UNSUPPORTED_CURVE_TYPE,  ///< generator for curve type is not implemented
  };

  enum class KeyValidatorError {
    WRONG_PUBLIC_KEY_SIZE = 1,     ///< public key has wrong size
    WRONG_PRIVATE_KEY_SIZE,        ///< private key has wrong size
    INVALID_PUBLIC_KEY,            ///< invalid public key
    INVALID_PRIVATE_KEY,           ///< private key cannot be decoded
    KEYS_DONT_MATCH,               ///< public key is not derived from private
    DIFFERENT_KEY_TYPES,           ///< key types in pair don't match
    UNSUPPORTED_CRYPTO_ALGORYTHM,  ///< crypto algorythm is not implemented
    UNKNOWN_CRYPTO_ALGORYTHM,      ///< unknown crypto algorythm
  };
}  // namespace libp2p::crypto

OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, CryptoProviderError)
OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, OpenSslError)
OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, HmacProviderError)
OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, RandomProviderError)
OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, KeyGeneratorError)
OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, KeyValidatorError)

#endif  // LIBP2P_CRYPTO_ERROR_HPP
