/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECIO_EXHANGE_MESSAGE_MARSHALLER_HPP
#define LIBP2P_SECIO_EXHANGE_MESSAGE_MARSHALLER_HPP

#include <vector>

#include <gsl/span>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/security/secio/exchange_message.hpp>

namespace libp2p::security::secio {

  namespace protobuf {
    class Exchange;
  }

  /**
   * Serializes and deserializes protobuf Exchange message in SECIO 1.0.0
   */
  class ExchangeMessageMarshaller {
   public:
    virtual ~ExchangeMessageMarshaller() = default;

    /**
     * Converts handy Exchange message to its protobuf counterpart
     * @param msg handy Exchange message
     * @return protobuf Exchange message
     */
    virtual protobuf::Exchange handyToProto(
        const ExchangeMessage &msg) const = 0;

    /**
     * Converts protobuf Exchange message to its handy counterpart
     * @param proto_msg protobuf Exchange message
     * @return handy Exchange message
     */
    virtual ExchangeMessage protoToHandy(
        const protobuf::Exchange &proto_msg) const = 0;

    /**
     * @param msg - handy Exchange message to be serialized
     * @returns a byte array with protobuf representation of the message
     */
    virtual outcome::result<std::vector<uint8_t>> marshal(
        const ExchangeMessage &msg) const = 0;

    /**
     * @param msg_bytes - bytes of protobuf representation of the message
     * @returns deserealized handy Exchange message
     */
    virtual outcome::result<ExchangeMessage> unmarshal(
        gsl::span<const uint8_t> msg_bytes) const = 0;
  };
}  // namespace libp2p::security::secio

#endif  // LIBP2P_SECIO_EXHANGE_MESSAGE_MARSHALLER_HPP
