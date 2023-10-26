/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/security/secio/propose_message_marshaller.hpp>

namespace libp2p::security::secio {

  class ProposeMessageMarshallerImpl : public ProposeMessageMarshaller {
   public:
    /**
     * Protobuf serialization errors
     */
    enum class Error {
      MESSAGE_SERIALIZING_ERROR = 1,
      MESSAGE_DESERIALIZING_ERROR,
    };

    protobuf::Propose handyToProto(const ProposeMessage &msg) const override;

    ProposeMessage protoToHandy(
        const protobuf::Propose &proto_msg) const override;

    outcome::result<std::vector<uint8_t>> marshal(
        const ProposeMessage &msg) const override;

    outcome::result<ProposeMessage> unmarshal(BytesIn msg_bytes) const override;
  };

}  // namespace libp2p::security::secio

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::secio,
                          ProposeMessageMarshallerImpl::Error);
