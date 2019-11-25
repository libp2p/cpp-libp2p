/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_SESSION_HOST_HPP
#define LIBP2P_KAD_SESSION_HOST_HPP

#include <libp2p/connection/stream.hpp>

namespace libp2p::protocol::kademlia {

  class KadSessionHost {
   public:
    virtual ~KadSessionHost() = default;

    virtual void onMessage(connection::Stream* from, struct Message&& msg) = 0;

    virtual void onCompleted(connection::Stream* from, outcome::result<void> res) = 0;

    virtual const struct KademliaConfig& config() = 0;
  };

} //namespace

#endif //LIBP2P_KAD_SESSION_HOST_HPP
