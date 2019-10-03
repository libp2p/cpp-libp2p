/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_EXCHANGE_MESSAGE_MARSHALLER_MOCK_HPP
#define LIBP2P_EXCHANGE_MESSAGE_MARSHALLER_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/security/plaintext/exchange_message.hpp"
#include "libp2p/security/plaintext/exchange_message_marshaller.hpp"

namespace libp2p::security::plaintext {
  class ExchangeMessageMarshallerMock : public ExchangeMessageMarshaller {
   public:
    ~ExchangeMessageMarshallerMock() override = default;

    MOCK_CONST_METHOD1(
        marshal,
        outcome::result<std::vector<uint8_t>>(const ExchangeMessage &));
    MOCK_CONST_METHOD1(
        unmarshal,
        outcome::result<std::pair<ExchangeMessage, crypto::ProtobufKey>>(
            gsl::span<const uint8_t>));
  };
}  // namespace libp2p::security::plaintext
#endif  // LIBP2P_EXCHANGE_MESSAGE_MARSHALLER_MOCK_HPP
