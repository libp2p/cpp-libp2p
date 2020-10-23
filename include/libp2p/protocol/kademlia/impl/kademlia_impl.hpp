/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL
#define LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL

#include <libp2p/protocol/kademlia/routing.hpp>
#include <libp2p/protocol/kademlia/message_observer.hpp>

namespace libp2p::protocol::kademlia {

class KademliaImpl final :
		public Routing,
		public MessageObserver {
public:
		~KademliaImpl() override = default;

//    /// Adds @param value corresponding to given @param key.
//    virtual outcome::result<void> putValue(Key key, Value value) = 0;
//
//    /// Searches for the @return value corresponding to given @param key.
//    virtual outcome::result<Value> getValue(const Key &key) = 0;
//
//

};

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL
