/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_CONTENTROUTINGTABLE
#define LIBP2P_PROTOCOL_KADEMLIA_CONTENTROUTINGTABLE

#include <libp2p/event/bus.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>

namespace libp2p::protocol::kademlia {

  namespace events {

    using ProvideContentChannel =
        event::channel_decl<struct ProvideContent,
                            std::pair<const ContentId &, const peer::PeerId &>>;

  }  // namespace events

  /**
   * @class ContentRoutingTable
   */
  struct ContentRoutingTable {
    virtual ~ContentRoutingTable() = default;

    virtual void addProvider(const ContentId &key,
                             const peer::PeerId &peer) = 0;

    virtual std::vector<PeerId> getProvidersFor(const ContentId &key,
                                                size_t limit = 0) const = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_PEERROUTINGTABLE
