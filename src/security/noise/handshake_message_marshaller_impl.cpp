/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/handshake_message_marshaller_impl.hpp>

#include <generated/security/noise/protobuf/noise.pb.h>
OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security::noise,
                            HandshakeMessageMarshallerImpl::Error, e) {
  using E = libp2p::security::noise::HandshakeMessageMarshallerImpl::Error;
  switch (e) {
    case E::MESSAGE_SERIALIZING_ERROR:
      return "Unable to serialize handshake payload message to protobuf";
    case E::MESSAGE_DESERIALIZING_ERROR:
      return "Unable to deserialize handshake payload message from protobuf";
  }
  return "Unknown error";
}

namespace libp2p::security::noise {

  HandshakeMessageMarshallerImpl::HandshakeMessageMarshallerImpl(
      std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller)
      : marshaller_{std::move(marshaller)} {};

  outcome::result<protobuf::NoiseHandshakePayload>
  HandshakeMessageMarshallerImpl::handyToProto(
      const HandshakeMessage &msg) const {
    protobuf::NoiseHandshakePayload proto_msg;

    OUTCOME_TRY(proto_pubkey_bytes, marshaller_->marshal(msg.identity_key));
    proto_msg.set_identity_key(proto_pubkey_bytes.key.data(),
                               proto_pubkey_bytes.key.size());
    proto_msg.set_identity_sig(msg.identity_sig.data(),
                               msg.identity_sig.size());
    proto_msg.set_data(msg.data.data(), msg.data.size());
    return proto_msg;
  }

  outcome::result<std::pair<HandshakeMessage, crypto::ProtobufKey>>
  HandshakeMessageMarshallerImpl::protoToHandy(
      const protobuf::NoiseHandshakePayload &proto_msg) const {
    common::ByteArray key_bytes{proto_msg.identity_key().begin(),
                                proto_msg.identity_key().end()};
    crypto::ProtobufKey proto_key{std::move(key_bytes)};
    OUTCOME_TRY(pubkey, marshaller_->unmarshalPublicKey(proto_key));

    return std::make_pair(
        HandshakeMessage{
            .identity_key = std::move(pubkey),
            .identity_sig = {proto_msg.identity_sig().begin(),
                             proto_msg.identity_sig().end()},
            .data = {proto_msg.data().begin(), proto_msg.data().end()}},
        std::move(proto_key));
  }

  outcome::result<common::ByteArray> HandshakeMessageMarshallerImpl::marshal(
      const HandshakeMessage &msg) const {
    OUTCOME_TRY(proto_msg, handyToProto(msg));
    common::ByteArray out_msg(proto_msg.ByteSizeLong());
    if (not proto_msg.SerializeToArray(out_msg.data(),
                                       static_cast<int>(out_msg.size()))) {
      return Error::MESSAGE_SERIALIZING_ERROR;
    }
    return out_msg;
  }

  outcome::result<std::pair<HandshakeMessage, crypto::ProtobufKey>>
  HandshakeMessageMarshallerImpl::unmarshal(
      gsl::span<const uint8_t> msg_bytes) const {
    protobuf::NoiseHandshakePayload proto_msg;
    if (not proto_msg.ParseFromArray(msg_bytes.data(),
                                     static_cast<int>(msg_bytes.size()))) {
      return Error::MESSAGE_DESERIALIZING_ERROR;
    }
    return protoToHandy(proto_msg);
  }
}  // namespace libp2p::security::noise
