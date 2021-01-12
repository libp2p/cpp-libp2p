/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_ERROR
#define LIBP2P_PROTOCOL_KADEMLIA_ERROR

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::protocol::kademlia {
  enum class Error {
    NO_PEERS = 1,
    MESSAGE_PARSE_ERROR,
    MESSAGE_DESERIALIZE_ERROR,
    MESSAGE_SERIALIZE_ERROR,
    MESSAGE_WRONG,
    UNEXPECTED_MESSAGE_TYPE,
    STREAM_RESET,
    VALUE_NOT_FOUND,
    CONTENT_VALIDATION_FAILED,
    TIMEOUT,
    IN_PROGRESS,
    FULFILLED,
    NOT_IMPLEMENTED,
    INTERNAL_ERROR,
    SESSION_CLOSED
  };
}

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia, Error);

#endif  // LIBP2P_PROTOCOL_KADEMLIA_ERROR
