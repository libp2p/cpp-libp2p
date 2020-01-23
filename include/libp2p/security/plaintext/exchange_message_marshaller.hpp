/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PLAINTEXT_EXCHANGE_MESSAGE_MARSHALLER_HPP
#define LIBP2P_PLAINTEXT_EXCHANGE_MESSAGE_MARSHALLER_HPP

#include <utility>
#include <vector>

#include <gsl/span>
#include <libp2p/crypto/protobuf/protobuf_key.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/security/plaintext/exchange_message.hpp>

namespace libp2p::security::plaintext {

  /**
   * Performs serializing of a Plaintext exchange message to Protobuf format and
   * deserializes it back
   */
  class ExchangeMessageMarshaller {
   public:
    virtual ~ExchangeMessageMarshaller() = default;

    /**
     * @param msg exchange message to be marshalled to Protobuf
     * @returns a byte array containing a Protobuf representation of an exchange
     * message
     */
    virtual outcome::result<std::vector<uint8_t>> marshal(
        const ExchangeMessage &msg) const = 0;

    /**
     * @param msg_bytes a byte array containing a Protobuf representation of an
     * exchange message
     * @returns a deserialized exchange message and a Protobuf representation of
     * the public key
     */
    virtual outcome::result<std::pair<ExchangeMessage, crypto::ProtobufKey>>
    unmarshal(gsl::span<const uint8_t> msg_bytes) const = 0;
  };

}  // namespace libp2p::security::plaintext

#endif  // LIBP2P_PLAINTEXT_EXCHANGE_MESSAGE_MARSHALLER_HPP
