/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>

#include <libp2p/crypto/protobuf/protobuf_key.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/security/noise/handshake_message.hpp>
#include <span>

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
    virtual outcome::result<Bytes> marshal(
        const HandshakeMessage &msg) const = 0;

    /**
     * @param msg_bytes of protobuf representation of the message
     * @return deserialized handy handshake message
     */
    virtual outcome::result<std::pair<HandshakeMessage, crypto::ProtobufKey>>
    unmarshal(BytesIn msg_bytes) const = 0;
  };
}  // namespace libp2p::security::noise
