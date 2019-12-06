/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_COMMON_HPP
#define LIBP2P_KADEMLIA_COMMON_HPP

#include <string>
#include <unordered_set>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/kademlia/content_address.hpp>

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
    TIMEOUT = 8
  };

  using libp2p::common::Hash256;

  /// DHT value
  using Value = std::vector<uint8_t>;

  /// Set of peer Ids
  using PeerIdSet = std::unordered_set<peer::PeerId>;

  /// Vector of peer Ids
  using PeerIdVec = std::vector<peer::PeerId>;

  /// Set of peer Infos
  using PeerInfoSet = std::unordered_set<peer::PeerInfo>;

}  // namespace libp2p::protocol::kademlia

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia, Error);

#endif  // LIBP2P_KADEMLIA_COMMON_HPP
