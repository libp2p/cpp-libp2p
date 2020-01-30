/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/plaintext/exchange_message_marshaller_impl.hpp>

#include <generated/security/plaintext/protobuf/plaintext.pb.h>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security::plaintext,
                            ExchangeMessageMarshallerImpl::Error, e) {
  using E = libp2p::security::plaintext::ExchangeMessageMarshallerImpl::Error;
  switch (e) {
    case E::PUBLIC_KEY_SERIALIZING_ERROR:
      return "Error while encoding the public key to Protobuf format";
    case E::MESSAGE_SERIALIZING_ERROR:
      return "Error while encoding the plaintext exchange message to Protobuf "
             "format";
    case E::PUBLIC_KEY_DESERIALIZING_ERROR:
      return "Error while decoding the public key from Protobuf format";
    case E::MESSAGE_DESERIALIZING_ERROR:
      return "Error while decoding the plaintext exchange message from "
             "Protobuf format";
  }
  return "Unknown error";
}

namespace libp2p::security::plaintext {

  ExchangeMessageMarshallerImpl::ExchangeMessageMarshallerImpl(
      std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller)
      : marshaller_{std::move(marshaller)} {};

  outcome::result<protobuf::Exchange>
  ExchangeMessageMarshallerImpl::handyToProto(
      const ExchangeMessage &msg) const {
    plaintext::protobuf::Exchange exchange_msg;

    OUTCOME_TRY(proto_pubkey_bytes, marshaller_->marshal(msg.pubkey));
    if (!exchange_msg.mutable_pubkey()->ParseFromArray(
            proto_pubkey_bytes.key.data(), proto_pubkey_bytes.key.size())) {
      return Error::PUBLIC_KEY_SERIALIZING_ERROR;
    }

    auto id = msg.peer_id.toMultihash().toBuffer();
    exchange_msg.set_id(id.data(), id.size());

    return outcome::success(std::move(exchange_msg));
  }

  outcome::result<std::pair<ExchangeMessage, crypto::ProtobufKey>>
  ExchangeMessageMarshallerImpl::protoToHandy(
      const protobuf::Exchange &proto_msg) const {
    std::vector<uint8_t> pubkey_message_bytes(
        proto_msg.pubkey().ByteSizeLong());
    if (!proto_msg.pubkey().SerializeToArray(pubkey_message_bytes.data(),
                                             pubkey_message_bytes.size())) {
      return Error::PUBLIC_KEY_SERIALIZING_ERROR;
    }
    crypto::ProtobufKey proto_pubkey{pubkey_message_bytes};
    OUTCOME_TRY(pubkey, marshaller_->unmarshalPublicKey(proto_pubkey));

    std::vector<uint8_t> peer_id_bytes(proto_msg.id().begin(),
                                       proto_msg.id().end());
    OUTCOME_TRY(peer_id, peer::PeerId::fromBytes(peer_id_bytes));

    return {ExchangeMessage{.pubkey = pubkey, .peer_id = peer_id},
            proto_pubkey};
  }

  outcome::result<std::vector<uint8_t>> ExchangeMessageMarshallerImpl::marshal(
      const ExchangeMessage &msg) const {
    OUTCOME_TRY(exchange_msg, handyToProto(msg));

    std::vector<uint8_t> out_msg(exchange_msg.ByteSizeLong());
    if (!exchange_msg.SerializeToArray(out_msg.data(), out_msg.size())) {
      return Error::MESSAGE_SERIALIZING_ERROR;
    }
    return out_msg;
  }

  outcome::result<std::pair<ExchangeMessage, crypto::ProtobufKey>>
  ExchangeMessageMarshallerImpl::unmarshal(
      gsl::span<const uint8_t> msg_bytes) const {
    plaintext::protobuf::Exchange exchange_msg;
    if (!exchange_msg.ParseFromArray(msg_bytes.data(), msg_bytes.size())) {
      return Error::PUBLIC_KEY_DESERIALIZING_ERROR;
    }

    return protoToHandy(exchange_msg);
  }

}  // namespace libp2p::security::plaintext
