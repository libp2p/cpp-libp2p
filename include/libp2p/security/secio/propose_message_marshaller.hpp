/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECIO_PROPOSE_MESSAGE_MARSHALLER_HPP
#define LIBP2P_SECIO_PROPOSE_MESSAGE_MARSHALLER_HPP

#include <vector>

#include <gsl/span>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/security/secio/propose_message.hpp>

namespace libp2p::security::secio {

  namespace protobuf {
    class Propose;
  }

  /**
   * Serializes and deserializes protobuf Propose message in SECIO 1.0.0
   */
  class ProposeMessageMarshaller {
   public:
    virtual ~ProposeMessageMarshaller() = default;

    /**
     * Converts handy Propose message to its protobuf counterpart
     * @param msg handy Propose message
     * @return protobuf Propose message
     */
    virtual protobuf::Propose handyToProto(const ProposeMessage &msg) const = 0;

    /**
     * Converts protobuf Propose message to its handy counterpart
     * @param proto_msg protobuf Propose message
     * @return handy Propose message
     */
    virtual ProposeMessage protoToHandy(
        const protobuf::Propose &proto_msg) const = 0;

    /**
     * @param msg - handy Propose message to be serialized
     * @returns a byte array with protobuf representation of the message
     */
    virtual outcome::result<std::vector<uint8_t>> marshal(
        const ProposeMessage &msg) const = 0;

    /**
     * @param msg_bytes - bytes of protobuf representation of the message
     * @returns deserealized handy Propose message
     */
    virtual outcome::result<ProposeMessage> unmarshal(
        gsl::span<const uint8_t> msg_bytes) const = 0;
  };

}  // namespace libp2p::security::secio

#endif  // LIBP2P_SECIO_PROPOSE_MESSAGE_MARSHALLER_HPP
