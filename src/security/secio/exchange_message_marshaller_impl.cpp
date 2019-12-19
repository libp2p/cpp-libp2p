/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/secio/exchange_message_marshaller_impl.hpp>

#include <generated/security/secio/protobuf/secio.pb.h>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security::secio,
                            ExchangeMessageMarshallerImpl::Error, e) {
  using E = libp2p::security::secio::ExchangeMessageMarshallerImpl::Error;
  switch (e) {
    case E::MESSAGE_SERIALIZING_ERROR:
      return "Error while encoding SECIO Exchange message to Protobuf format";
    case E::MESSAGE_DESERIALIZING_ERROR:
      return "Error while decoding SECIO Exchange message from Protobuf format";
  }
  return "Unknown error";
}

namespace libp2p::security::secio {

  protobuf::Exchange ExchangeMessageMarshallerImpl::handyToProto(
      const ExchangeMessage &msg) const {
    protobuf::Exchange exchange_msg;

    exchange_msg.set_epubkey(msg.epubkey.data(), msg.epubkey.size());
    exchange_msg.set_signature(msg.signature.data(), msg.signature.size());

    return exchange_msg;
  }

  ExchangeMessage ExchangeMessageMarshallerImpl::protoToHandy(
      const protobuf::Exchange &proto_msg) const {
    return ExchangeMessage{
        .epubkey = {proto_msg.epubkey().begin(), proto_msg.epubkey().end()},
        .signature = {proto_msg.signature().begin(),
                      proto_msg.signature().end()}};
  }

  outcome::result<std::vector<uint8_t>> ExchangeMessageMarshallerImpl::marshal(
      const ExchangeMessage &msg) const {
    auto exchange_msg{handyToProto(msg)};

    std::vector<uint8_t> out_msg(exchange_msg.ByteSizeLong());
    if (!exchange_msg.SerializeToArray(out_msg.data(), out_msg.size())) {
      return Error::MESSAGE_SERIALIZING_ERROR;
    }
    return out_msg;
  }

  outcome::result<ExchangeMessage> ExchangeMessageMarshallerImpl::unmarshal(
      gsl::span<const uint8_t> msg_bytes) const {
    protobuf::Exchange exchange_msg;
    if (!exchange_msg.ParseFromArray(msg_bytes.data(), msg_bytes.size())) {
      return Error::MESSAGE_DESERIALIZING_ERROR;
    }

    return protoToHandy(exchange_msg);
  }

}  // namespace libp2p::security::secio
