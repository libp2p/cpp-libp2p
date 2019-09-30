/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>

#include <libp2p/crypto/common.hpp>
#include <libp2p/crypto/key_generator.hpp>
#include <generated/crypto/protobuf/keys.pb.h>

namespace libp2p::crypto::marshaller {
  namespace {
    /**
     * @brief converts Key::Type to protobuf::KeyType
     * @param key_type common key type value
     * @return protobuf key type value
     */
    outcome::result<protobuf::KeyType> marshalKeyType(Key::Type key_type) {
      switch (key_type) {
        case Key::Type::UNSPECIFIED:
          return protobuf::KeyType::UNSPECIFIED;
        case Key::Type::RSA1024:
          return protobuf::KeyType::RSA1024;
        case Key::Type::RSA2048:
          return protobuf::KeyType::RSA2048;
        case Key::Type::RSA4096:
          return protobuf::KeyType::RSA4096;
        case Key::Type::ED25519:
          return protobuf::KeyType::ED25519;
        case Key::Type::SECP256K1:
          return protobuf::KeyType::SECP256K1;
      }

      return CryptoProviderError::UNKNOWN_KEY_TYPE;
    }

    /**
     * @brief converts protobuf::KeyType to Key::Type
     * @param key_type protobuf key type value
     * @return common key type value
     */
    outcome::result<Key::Type> unmarshalKeyType(protobuf::KeyType key_type) {
      switch (key_type) {
        case protobuf::KeyType::UNSPECIFIED:
          return Key::Type::UNSPECIFIED;
        case protobuf::KeyType::RSA1024:
          return Key::Type::RSA1024;
        case protobuf::KeyType::RSA2048:
          return Key::Type::RSA2048;
        case protobuf::KeyType::RSA4096:
          return Key::Type::RSA4096;
        case protobuf::KeyType::ED25519:
          return Key::Type::ED25519;
        case protobuf::KeyType::SECP256K1:
          return Key::Type::SECP256K1;
        default:
          return CryptoProviderError::UNKNOWN_KEY_TYPE;
      }
    }
  }  // namespace

  KeyMarshallerImpl::KeyMarshallerImpl(
      std::shared_ptr<validator::KeyValidator> key_validator)
      : key_validator_{std::move(key_validator)} {}

  outcome::result<KeyMarshallerImpl::ByteArray> KeyMarshallerImpl::marshal(
      const PublicKey &key) const {
    protobuf::PublicKey protobuf_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.type));
    protobuf_key.set_key_type(key_type);
    protobuf_key.set_key_value(key.data.data(), key.data.size());

    auto string = protobuf_key.SerializeAsString();
    KeyMarshallerImpl::ByteArray out(string.begin(), string.end());
    return KeyMarshallerImpl::ByteArray(string.begin(), string.end());
  }

  outcome::result<KeyMarshallerImpl::ByteArray> KeyMarshallerImpl::marshal(
      const PrivateKey &key) const {
    // TODO(Harrm): Check if it's a typo
    protobuf::PrivateKey protobuf_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.type));
    protobuf_key.set_key_type(key_type);
    protobuf_key.set_key_value(key.data.data(), key.data.size());

    auto string = protobuf_key.SerializeAsString();
    return KeyMarshallerImpl::ByteArray(string.begin(), string.end());
  }

  outcome::result<PublicKey> KeyMarshallerImpl::unmarshalPublicKey(
      const KeyMarshallerImpl::ByteArray &key_bytes) const {
    protobuf::PublicKey protobuf_key;
    if (!protobuf_key.ParseFromArray(key_bytes.data(), key_bytes.size())) {
      return CryptoProviderError::FAILED_UNMARSHAL_DATA;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(protobuf_key.key_type()));
    KeyMarshallerImpl::ByteArray key_value(protobuf_key.key_value().begin(),
                                           protobuf_key.key_value().end());
    auto key = PublicKey{{key_type, key_value}};
    OUTCOME_TRY(key_validator_->validate(key));

    return key;
  }

  outcome::result<PrivateKey> KeyMarshallerImpl::unmarshalPrivateKey(
      const KeyMarshallerImpl::ByteArray &key_bytes) const {
    protobuf::PublicKey protobuf_key;
    if (!protobuf_key.ParseFromArray(key_bytes.data(), key_bytes.size())) {
      return CryptoProviderError::FAILED_UNMARSHAL_DATA;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(protobuf_key.key_type()));
    KeyMarshallerImpl::ByteArray key_value(protobuf_key.key_value().begin(),
                                           protobuf_key.key_value().end());
    auto key = PrivateKey{{key_type, key_value}};
    OUTCOME_TRY(key_validator_->validate(key));

    return key;
  }

}  // namespace libp2p::crypto::marshaller
