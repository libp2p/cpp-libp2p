/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_ERROR
#define LIBP2P_PROTOCOL_KADEMLIA_ERROR

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::protocol::kademlia {
  enum class Error {
    SUCCESS = 0,
    NO_PEERS = 1,
    MESSAGE_PARSE_ERROR = 2,
    MESSAGE_SERIALIZE_ERROR = 3,
    UNEXPECTED_MESSAGE_TYPE = 4,
    STREAM_RESET = 5,
    VALUE_NOT_FOUND = 6,
    CONTENT_VALIDATION_FAILED = 7,
    TIMEOUT = 8,
    IN_PROGRESS,
		FULFILLED
  };
}

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia, Error);

#endif  // LIBP2P_PROTOCOL_KADEMLIA_ERROR
