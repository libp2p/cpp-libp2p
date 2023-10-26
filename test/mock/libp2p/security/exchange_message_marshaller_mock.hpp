/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/security/plaintext/exchange_message_marshaller.hpp>

#include <gmock/gmock.h>

#include <generated/security/plaintext/protobuf/plaintext.pb.h>
#include <libp2p/security/plaintext/exchange_message.hpp>

namespace libp2p::security::plaintext {
  class ExchangeMessageMarshallerMock : public ExchangeMessageMarshaller {
   public:
    ~ExchangeMessageMarshallerMock() override = default;

    MOCK_CONST_METHOD1(
        handyToProto,
        outcome::result<protobuf::Exchange>(const ExchangeMessage &));

    MOCK_CONST_METHOD1(
        protoToHandy,
        outcome::result<std::pair<ExchangeMessage, crypto::ProtobufKey>>(
            const protobuf::Exchange &));

    MOCK_CONST_METHOD1(
        marshal,
        outcome::result<std::vector<uint8_t>>(const ExchangeMessage &));
    MOCK_CONST_METHOD1(
        unmarshal,
        outcome::result<std::pair<ExchangeMessage, crypto::ProtobufKey>>(
            BytesIn));
  };
}  // namespace libp2p::security::plaintext
