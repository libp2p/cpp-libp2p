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
  }
  return "Unknown error";
}

namespace libp2p::security::plaintext {

  ExchangeMessageMarshallerImpl::ExchangeMessageMarshallerImpl(
      std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller)
      : marshaller_{std::move(marshaller)} {};

  outcome::result<std::vector<uint8_t>> ExchangeMessageMarshallerImpl::marshal(
      const ExchangeMessage &msg) const {
    plaintext::protobuf::Exchange exchange_msg;
    auto id = msg.peer_id.toMultihash().toBuffer();
    exchange_msg.set_id(id.data(), id.size());

    auto pubkey = msg.pubkey;
    OUTCOME_TRY(pub_key, marshaller_->marshal(pubkey));
    std::string str_pubkey(pub_key.begin(), pub_key.end());
    auto pubkey_msg = std::make_unique<crypto::protobuf::PublicKey>();
    if (!pubkey_msg->ParseFromString(str_pubkey)) {
      return Error::PUBLIC_KEY_SERIALIZING_ERROR;
    }
    exchange_msg.set_allocated_pubkey(pubkey_msg.get());

    std::vector<uint8_t> out_msg(exchange_msg.ByteSizeLong());
    if (!exchange_msg.SerializeToArray(out_msg.data(), out_msg.size())) {
      return Error::MESSAGE_SERIALIZING_ERROR;
    }
    exchange_msg.release_pubkey();
    return out_msg;
  }

  outcome::result<ExchangeMessage> ExchangeMessageMarshallerImpl::unmarshal(
      gsl::span<const uint8_t> msg_bytes) const {
    plaintext::protobuf::Exchange exchange_msg;
    exchange_msg.ParseFromArray(msg_bytes.data(), msg_bytes.size());
    std::vector<uint8_t> pubkey_bytes(exchange_msg.pubkey().ByteSizeLong());
    exchange_msg.pubkey().SerializeToArray(pubkey_bytes.data(),
                                           pubkey_bytes.size());
    OUTCOME_TRY(pubkey, marshaller_->unmarshalPublicKey(pubkey_bytes));

    std::vector<uint8_t> peer_id_bytes(exchange_msg.id().begin(),
                                       exchange_msg.id().end());
    OUTCOME_TRY(peer_id, peer::PeerId::fromBytes(peer_id_bytes));
    return ExchangeMessage{pubkey, peer_id};
  }

}  // namespace libp2p::security::plaintext
