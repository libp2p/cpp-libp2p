/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/key_marshaller.hpp>

#include <gmock/gmock.h>

namespace libp2p::crypto::marshaller {

  struct KeyMarshallerMock : public KeyMarshaller {
    ~KeyMarshallerMock() override = default;

    MOCK_CONST_METHOD1(marshal,
                       outcome::result<ProtobufKey>(const PublicKey &));

    MOCK_CONST_METHOD1(marshal,
                       outcome::result<ProtobufKey>(const PrivateKey &));

    MOCK_CONST_METHOD1(unmarshalPublicKey,
                       outcome::result<PublicKey>(const ProtobufKey &));

    MOCK_CONST_METHOD1(unmarshalPrivateKey,
                       outcome::result<PrivateKey>(const ProtobufKey &));
  };

}  // namespace libp2p::crypto::marshaller
