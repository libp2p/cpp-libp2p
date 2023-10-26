/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/key.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/crypto/key_validator.hpp>

namespace libp2p::crypto::marshaller {
  class KeyMarshallerImpl : public KeyMarshaller {
   public:
    explicit KeyMarshallerImpl(
        std::shared_ptr<validator::KeyValidator> key_validator);

    ~KeyMarshallerImpl() override = default;

    outcome::result<ProtobufKey> marshal(const PublicKey &key) const override;

    outcome::result<ProtobufKey> marshal(const PrivateKey &key) const override;

    outcome::result<PublicKey> unmarshalPublicKey(
        const ProtobufKey &key) const override;

    outcome::result<PrivateKey> unmarshalPrivateKey(
        const ProtobufKey &key) const override;

   private:
    std::shared_ptr<validator::KeyValidator> key_validator_;
  };
}  // namespace libp2p::crypto::marshaller
