/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_MESSAGEOBSERVER
#define LIBP2P_PROTOCOL_KADEMLIA_MESSAGEOBSERVER

#include <functional>
#include <libp2p/connection/stream.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>

namespace libp2p::protocol::kademlia {

  class MessageObserver {
   public:
    virtual ~MessageObserver() = default;

    /// Handles inbound message
    virtual void onMessage(connection::Stream *from, struct Message &&msg) = 0;

    /// Handles closed connection
    virtual void onCompleted(connection::Stream *from,
                             outcome::result<void> res) = 0;

    // request handlers
    virtual bool onPutValue(Message &msg) = 0;
    virtual bool onGetValue(Message &msg) = 0;
    virtual bool onAddProvider(Message &msg) = 0;
    virtual bool onGetProviders(Message &msg) = 0;
    virtual bool onFindNode(Message &msg) = 0;
    virtual bool onPing(Message &msg) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_MESSAGEOBSERVER
