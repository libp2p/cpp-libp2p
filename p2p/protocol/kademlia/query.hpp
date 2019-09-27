/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_QUERY_HPP
#define LIBP2P_KAD_QUERY_HPP

#include <unordered_set>

#include <boost/optional.hpp>
#include "peer/peer_info.hpp"
#include "protocol/kademlia/common.hpp"

namespace libp2p::protocol::kademlia {

  struct QueryResult {
    std::vector<peer::PeerInfo> closerPeers{};
    boost::optional<peer::PeerInfo> peer{};
    bool success = false;
  };

  using QueryResultFunc = std::function<void(outcome::result<QueryResult>)>;
  using QueryFunc = std::function<void(const Key &, QueryResultFunc)>;

  struct Query {
    Key key;
    QueryFunc func;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_QUERY_HPP
