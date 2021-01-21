/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONTENT_ROUTING_HPP
#define LIBP2P_CONTENT_ROUTING_HPP

#include <libp2p/event/subscription.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/kad/common.hpp>

namespace libp2p::protocol::kademlia {

  /// ContentRouting is used to find information about who has what content.
  struct ContentRouting {
    virtual ~ContentRouting() = default;

    using PeerInfoFunc = std::function<void(peer::PeerInfo)>;
    using ProvideResultFunc = std::function<void(outcome::result<void>)>;

    /**
     * @brief Provide adds the given CID to the content routing system.
     * @param cid content id
     * @param broadcast if true, \provide announces it, otherwise it is just
     * kept in the local accounting of which objects are being provided.
     * @param f if error occurred, it will be passed to this callback.
     */
    virtual void provide(Cid cid, bool broadcast, ProvideResultFunc f) = 0;

    /**
     * @brief Search for peers who are able to provide a given key.
     * @param cid content id
     * @param f callback that is executed in case of error (not found), or
     * success.
     * @return \findProviders works until subscription is active.
     */
    virtual event::Subscription findProviders(Cid cid, PeerInfoFunc f) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_CONTENT_ROUTING_HPP
