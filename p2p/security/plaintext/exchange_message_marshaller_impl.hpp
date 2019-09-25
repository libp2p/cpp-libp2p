/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXCHANGE_MESSAGE_MARSHALLER_IMPL_HPP
#define KAGOME_EXCHANGE_MESSAGE_MARSHALLER_IMPL_HPP

#include <vector>

#include "crypto/key_marshaller.hpp"
#include "security/plaintext/exchange_message.hpp"
#include "security/plaintext/exchange_message_marshaller.hpp"

namespace libp2p::security::plaintext {

  class ExchangeMessageMarshallerImpl : public ExchangeMessageMarshaller {
   public:
    /**
     * Protobuf serialization errors
     */
    enum class Error {
      PUBLIC_KEY_SERIALIZING_ERROR = 1,
      MESSAGE_SERIALIZING_ERROR
      };

    explicit ExchangeMessageMarshallerImpl(
        std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller);

    outcome::result<std::vector<uint8_t>> marshal(
        const ExchangeMessage &msg) const override;

    outcome::result<ExchangeMessage> unmarshal(
        gsl::span<const uint8_t> msg_bytes) const override;

   private:
    std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller_;
  };

}  // namespace libp2p::security::plaintext

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::plaintext,
                          ExchangeMessageMarshallerImpl::Error);

#endif  // KAGOME_EXCHANGE_MESSAGE_MARSHALLER_IMPL_HPP
