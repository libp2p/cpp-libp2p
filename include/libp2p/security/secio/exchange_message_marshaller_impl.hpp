/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECIO_EXCHANGE_MESSAGE_MARSHALLER_IMPL_HPP
#define LIBP2P_SECIO_EXCHANGE_MESSAGE_MARSHALLER_IMPL_HPP

#include <libp2p/security/secio/exchange_message_marshaller.hpp>

namespace libp2p::security::secio {

  class ExchangeMessageMarshallerImpl : public ExchangeMessageMarshaller {
   public:
    /**
     * Protobuf serialization errors
     */
    enum class Error {
      MESSAGE_SERIALIZING_ERROR = 1,
      MESSAGE_DESERIALIZING_ERROR,
    };

    protobuf::Exchange handyToProto(const ExchangeMessage &msg) const override;

    ExchangeMessage protoToHandy(
        const protobuf::Exchange &proto_msg) const override;

    outcome::result<std::vector<uint8_t>> marshal(
        const ExchangeMessage &msg) const override;

    outcome::result<ExchangeMessage> unmarshal(
        gsl::span<const uint8_t> msg_bytes) const override;
  };

}  // namespace libp2p::security::secio

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::secio,
                          ExchangeMessageMarshallerImpl::Error);

#endif  // LIBP2P_SECIO_EXCHANGE_MESSAGE_MARSHALLER_IMPL_HPP
