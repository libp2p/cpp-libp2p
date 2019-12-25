/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/secio/propose_message_marshaller_impl.hpp>

#include <generated/security/secio/protobuf/secio.pb.h>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security::secio,
                            ProposeMessageMarshallerImpl::Error, e) {
  using E = libp2p::security::secio::ProposeMessageMarshallerImpl::Error;
  switch (e) {
    case E::MESSAGE_SERIALIZING_ERROR:
      return "Error while encoding SECIO Propose message to Protobuf format";
    case E::MESSAGE_DESERIALIZING_ERROR:
      return "Error while decoding SECIO Propose message from Protobuf format";
  }
  return "Unknown error";
}

namespace libp2p::security::secio {

  protobuf::Propose ProposeMessageMarshallerImpl::handyToProto(
      const ProposeMessage &msg) const {
    protobuf::Propose propose_msg;

    propose_msg.set_rand(msg.rand.data(), msg.rand.size());
    propose_msg.set_pubkey(msg.pubkey.data(), msg.pubkey.size());
    propose_msg.set_exchanges(msg.exchanges);
    propose_msg.set_ciphers(msg.ciphers);
    propose_msg.set_hashes(msg.hashes);

    return propose_msg;
  }

  ProposeMessage ProposeMessageMarshallerImpl::protoToHandy(
      const protobuf::Propose &proto_msg) const {
    return ProposeMessage{
        .rand = {proto_msg.rand().begin(), proto_msg.rand().end()}, // NOLINT
        .pubkey = {proto_msg.pubkey().begin(), proto_msg.pubkey().end()},
        .exchanges = proto_msg.exchanges(),
        .ciphers = proto_msg.ciphers(),
        .hashes = proto_msg.hashes()};
  }

  outcome::result<std::vector<uint8_t>> ProposeMessageMarshallerImpl::marshal(
      const ProposeMessage &msg) const {
    auto propose_msg{handyToProto(msg)};

    std::vector<uint8_t> out_msg(propose_msg.ByteSizeLong());
    if (!propose_msg.SerializeToArray(out_msg.data(), out_msg.size())) {
      return Error::MESSAGE_SERIALIZING_ERROR;
    }
    return out_msg;
  }

  outcome::result<ProposeMessage> ProposeMessageMarshallerImpl::unmarshal(
      gsl::span<const uint8_t> msg_bytes) const {
    protobuf::Propose propose_msg;
    if (!propose_msg.ParseFromArray(msg_bytes.data(), msg_bytes.size())) {
      return Error::MESSAGE_DESERIALIZING_ERROR;
    }
    return protoToHandy(propose_msg);
  }

}  // namespace libp2p::security::secio
