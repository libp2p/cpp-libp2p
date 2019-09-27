/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_KAD_HPP
#define LIBP2P_KADEMLIA_KAD_HPP

#include "network/network.hpp"
#include "protocol/kademlia/config.hpp"
#include "protocol/kademlia/value_store.hpp"
#include "routing/content_routing.hpp"
#include "routing/peer_routing.hpp"

namespace libp2p::protocol::kademlia {

  /**
   * @class Kad
   *
   * Entrypoint to a kademlia network.
   */
  struct Kad
      : public PeerRouting /*, public ContentRouting, public ValueStore */ {
    ~Kad() override = default;

    enum class Error {
      SUCCESS = 0,
      NO_PEERS = 1
    };
  };

}  // namespace libp2p::protocol::kademlia

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia, Kad::Error);

#endif  // LIBP2P_KADEMLIA_KAD_HPP
