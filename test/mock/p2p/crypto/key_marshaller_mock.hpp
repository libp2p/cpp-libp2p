/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEY_MARSHALLER_MOCK_HPP
#define KAGOME_KEY_MARSHALLER_MOCK_HPP

#include <gmock/gmock.h>
#include "p2p/crypto/key_marshaller.hpp"

namespace libp2p::crypto::marshaller {

  struct KeyMarshallerMock : public KeyMarshaller {
    ~KeyMarshallerMock() override = default;

    MOCK_CONST_METHOD1(
        marshal, outcome::result<KeyMarshaller::ByteArray>(const PublicKey &));

    MOCK_CONST_METHOD1(
        marshal, outcome::result<KeyMarshaller::ByteArray>(const PrivateKey &));

    MOCK_CONST_METHOD1(
        unmarshalPublicKey,
        outcome::result<PublicKey>(const KeyMarshaller::ByteArray &));

    MOCK_CONST_METHOD1(
        unmarshalPrivateKey,
        outcome::result<PrivateKey>(const KeyMarshaller::ByteArray &));
  };

}  // namespace libp2p::crypto::marshaller

#endif  // KAGOME_KEY_MARSHALLER_MOCK_HPP
