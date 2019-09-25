/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_HPP

#include "crypto/error.hpp"
#include "crypto/key.hpp"

namespace libp2p::crypto::marshaller {
/**
 * @class KeyMarshaller provides methods for serializing and deserializing
 * private and public keys from/to google-protobuf format
 */
class KeyMarshaller {
 public:
  using ByteArray = std::vector<uint8_t>;
  virtual ~KeyMarshaller() = default;
  /**
   * Convert the public key into Protobuf representation
   * @param key - public key to be mashalled
   * @return bytes of Protobuf object if marshalling was successful, error
   * otherwise
   */
  virtual outcome::result<ByteArray> marshal(const PublicKey &key) const = 0;

  /**
   * Convert the private key into Protobuf representation
   * @param key - public key to be mashalled
   * @return bytes of Protobuf object if marshalling was successful, error
   * otherwise
   */
  virtual outcome::result<ByteArray> marshal(const PrivateKey &key) const = 0;

  /**
   * Convert Protobuf representation of public key into the object
   * @param key_bytes - bytes of the public key
   * @return public key in case of success, error otherwise
   */
  virtual outcome::result<PublicKey> unmarshalPublicKey(
      const ByteArray &key_bytes) const = 0;

  /**
   * Convert Protobuf representation of private key into the object
   * @param key_bytes - bytes of the private key
   * @return private key in case of success, error otherwise
   */
  virtual outcome::result<PrivateKey> unmarshalPrivateKey(
      const ByteArray &key_bytes) const = 0;
};
}  // namespace libp2p::crypto::key_marshaller

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_HPP
