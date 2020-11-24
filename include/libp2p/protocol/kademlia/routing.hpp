/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_ROUTING
#define LIBP2P_PROTOCOL_KADEMLIA_ROUTING

#include <libp2p/protocol/kademlia/content_routing.hpp>
#include <libp2p/protocol/kademlia/peer_routing.hpp>
#include <libp2p/protocol/kademlia/value_store.hpp>

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::protocol::kademlia {

  class Routing : public ValueStore, public ContentRouting, public PeerRouting {
   public:
    ~Routing() override = default;

    /**
     * Kicks off the bootstrap process
     * For each call, we generate a random node ID and we look it up via the
     * process defined in 'Node lookups'.
     */
    virtual outcome::result<void> bootstrap() = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_ROUTING
