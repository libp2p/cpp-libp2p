/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_KADEMLIA
#define LIBP2P_PROTOCOL_KADEMLIA_KADEMLIA

#include <libp2p/protocol/kademlia/routing.hpp>

namespace libp2p::protocol::kademlia {

  class Kademlia : public Routing {
   public:
    virtual ~Kademlia() = default;

    virtual void start() = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_KADEMLIA
