/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_MESSAGE_MARSHALLER_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_MESSAGE_MARSHALLER_HPP

#include <tuple>

#include <gsl/span>
#include <libp2p/crypto/protobuf/protobuf_key.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/security/noise/handshake_message.hpp>

namespace libp2p::security::noise {

  namespace protobuf {
    class NoiseHandshakePayload;
  }

  /**
   * Serializes and deserializes protobuf NoiseHandshakePayload message in Noise
   * protocol
   */
  class HandshakeMessageMarshaller {
   public:
    virtual ~HandshakeMessageMarshaller() = default;

    /**
     * Converts handy handshake message to its protobuf counterpart
     * @param msg handy handshake message
     * @return protobuf handshake message
     */
    virtual outcome::result<protobuf::NoiseHandshakePayload> handyToProto(
        const HandshakeMessage &msg) const = 0;

    /**
     * Converts protobuf handshake message to its handy counterpart
     * @param proto_msg protobuf handshake message
     * @return handy handshake message
     */
    virtual outcome::result<std::pair<HandshakeMessage, crypto::ProtobufKey>>
    protoToHandy(const protobuf::NoiseHandshakePayload &proto_msg) const = 0;

    /**
     * @param msg handy handshake message to be serialized
     * @return a byte array with protobuf representation of the message
     */
    virtual outcome::result<common::ByteArray> marshal(
        const HandshakeMessage &msg) const = 0;

    /**
     * @param msg_bytes of protobuf representation of the message
     * @return deserialized handy handshake message
     */
    virtual outcome::result<std::pair<HandshakeMessage, crypto::ProtobufKey>>
    unmarshal(gsl::span<const uint8_t> msg_bytes) const = 0;
  };
}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_MESSAGE_MARSHALLER_HPP
