/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_HPP
#define LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_HPP

#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/key.hpp>
#include <libp2p/crypto/protobuf/protobuf_key.hpp>

namespace libp2p::crypto::marshaller {
  /**
   * @class KeyMarshaller provides methods for serializing and deserializing
   * private and public keys from/to google-protobuf format
   */
  class KeyMarshaller {
   public:
    virtual ~KeyMarshaller() = default;

    /**
     * Convert the public key into Protobuf representation
     * @param key - public key to be mashalled
     * @return bytes of Protobuf object if marshalling was successful, error
     * otherwise
     */
    virtual outcome::result<ProtobufKey> marshal(
        const PublicKey &key) const = 0;

    /**
     * Convert the private key into Protobuf representation
     * @param key - public key to be mashalled
     * @return bytes of Protobuf object if marshalling was successful, error
     * otherwise
     */
    virtual outcome::result<ProtobufKey> marshal(
        const PrivateKey &key) const = 0;
    /**
     * Convert Protobuf representation of public key into the object
     * @param key_bytes - bytes of the public key
     * @return public key in case of success, error otherwise
     */
    virtual outcome::result<PublicKey> unmarshalPublicKey(
        const ProtobufKey &key) const = 0;

    /**
     * Convert Protobuf representation of private key into the object
     * @param key_bytes - bytes of the private key
     * @return private key in case of success, error otherwise
     */
    virtual outcome::result<PrivateKey> unmarshalPrivateKey(
        const ProtobufKey &key) const = 0;
  };
}  // namespace libp2p::crypto::marshaller

#endif  // LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_HPP
