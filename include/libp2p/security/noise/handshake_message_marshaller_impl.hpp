/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_MESSAGE_MARSHALLER_IMPL_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_MESSAGE_MARSHALLER_IMPL_HPP

#include <vector>

#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/security/noise/handshake_message_marshaller.hpp>

namespace libp2p::security::noise {
  class HandshakeMessageMarshallerImpl : public HandshakeMessageMarshaller {
   public:
    enum class Error {
      MESSAGE_SERIALIZING_ERROR = 1,
      MESSAGE_DESERIALIZING_ERROR,
    };

    explicit HandshakeMessageMarshallerImpl(
        std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller);

    outcome::result<protobuf::NoiseHandshakePayload> handyToProto(
        const HandshakeMessage &msg) const override;

    outcome::result<std::pair<HandshakeMessage, crypto::ProtobufKey>> protoToHandy(
        const protobuf::NoiseHandshakePayload &proto_msg) const override;

    outcome::result<common::ByteArray> marshal(
        const HandshakeMessage &msg) const override;

    outcome::result<std::pair<HandshakeMessage, crypto::ProtobufKey>> unmarshal(
        gsl::span<const uint8_t> msg_bytes) const override;

   private:
    std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller_;
  };
}  // namespace libp2p::security::noise

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::noise,
                          HandshakeMessageMarshallerImpl::Error);

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_MESSAGE_MARSHALLER_IMPL_HPP
