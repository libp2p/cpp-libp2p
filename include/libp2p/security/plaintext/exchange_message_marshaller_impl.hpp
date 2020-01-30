/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PLAINTEXT_EXCHANGE_MESSAGE_MARSHALLER_IMPL_HPP
#define LIBP2P_PLAINTEXT_EXCHANGE_MESSAGE_MARSHALLER_IMPL_HPP

#include <vector>

#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/security/plaintext/exchange_message.hpp>
#include <libp2p/security/plaintext/exchange_message_marshaller.hpp>

namespace libp2p::crypto::protobuf {
  class PublicKey;
}

namespace libp2p::security::plaintext {

  class ExchangeMessageMarshallerImpl : public ExchangeMessageMarshaller {
   public:
    /**
     * Protobuf serialization errors
     */
    enum class Error {
      PUBLIC_KEY_SERIALIZING_ERROR = 1,
      MESSAGE_SERIALIZING_ERROR,
      PUBLIC_KEY_DESERIALIZING_ERROR,
      MESSAGE_DESERIALIZING_ERROR,
    };

    explicit ExchangeMessageMarshallerImpl(
        std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller);

    outcome::result<protobuf::Exchange> handyToProto(
        const ExchangeMessage &msg) const override;

    outcome::result<std::pair<ExchangeMessage, crypto::ProtobufKey>>
    protoToHandy(const protobuf::Exchange &proto_msg) const override;

    outcome::result<std::vector<uint8_t>> marshal(
        const ExchangeMessage &msg) const override;

    outcome::result<std::pair<ExchangeMessage, crypto::ProtobufKey>> unmarshal(
        gsl::span<const uint8_t> msg_bytes) const override;

   private:
    std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller_;
  };

}  // namespace libp2p::security::plaintext

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::plaintext,
                          ExchangeMessageMarshallerImpl::Error);

#endif  // LIBP2P_PLAINTEXT_EXCHANGE_MESSAGE_MARSHALLER_IMPL_HPP
