/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_QUERY_RUNNER_HPP
#define LIBP2P_KAD_QUERY_RUNNER_HPP

#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/query.hpp>

namespace libp2p::protocol::kademlia {

  struct QueryRunner {
    virtual ~QueryRunner() = default;

    using PeerInfoResult = outcome::result<peer::PeerInfo>;
    using PeerInfoResultFunc = std::function<void(PeerInfoResult)>;

    virtual void run(Query query, PeerIdVec peers, PeerInfoResultFunc f) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_QUERY_RUNNER_HPP
