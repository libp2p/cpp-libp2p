/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>

#include <libp2p/common/types.hpp>
#include <libp2p/crypto/key_type.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <qtils/bytes.hpp>

namespace libp2p::crypto {

  using Buffer = libp2p::Bytes;

  struct Key {
    /**
     * Supported types of all keys
     */
    using Type = KeyType;

    Type type = Type::UNSPECIFIED;  ///< key type
    std::vector<uint8_t> data{};    ///< key content
  };

  inline bool operator==(const Key &lhs, const Key &rhs) {
    return lhs.type == rhs.type && lhs.data == rhs.data;
  }

  inline bool operator!=(const Key &lhs, const Key &rhs) {
    return !(lhs == rhs);
  }

  struct PublicKey : public Key {};

  struct PrivateKey : public Key {};

  struct KeyPair {
    template <typename T>
      requires(requires { T::key_type; })
    KeyPair(const T &v)
        : publicKey{T::key_type, qtils::asVec(v.public_key)},
          privateKey{T::key_type, qtils::asVec(v.private_key)} {}

    PublicKey publicKey;
    PrivateKey privateKey;
  };

  inline bool operator==(const KeyPair &a, const KeyPair &b) {
    return a.publicKey == b.publicKey && a.privateKey == b.privateKey;
  }

  /**
   * Result of ephemeral key generation
   */
  struct EphemeralKeyPair {
    Buffer ephemeral_public_key;
    std::function<outcome::result<Buffer>(Buffer)> shared_secret_generator;
  };

  /**
   * Type of the stretched key
   */
  struct StretchedKey {
    Buffer iv;
    Buffer cipher_key;
    Buffer mac_key;
  };

}  // namespace libp2p::crypto

namespace std {
  template <>
  struct hash<libp2p::crypto::Key> {
    size_t operator()(const libp2p::crypto::Key &x) const;
  };

  template <>
  struct hash<libp2p::crypto::PrivateKey> {
    size_t operator()(const libp2p::crypto::PrivateKey &x) const;
  };

  template <>
  struct hash<libp2p::crypto::PublicKey> {
    size_t operator()(const libp2p::crypto::PublicKey &x) const;
  };

  template <>
  struct hash<libp2p::crypto::KeyPair> {
    size_t operator()(const libp2p::crypto::KeyPair &x) const;
  };
}  // namespace std
