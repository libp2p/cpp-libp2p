/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_SESSION_HOST_HPP
#define LIBP2P_KAD_SESSION_HOST_HPP

#include <libp2p/connection/stream.hpp>
#include <libp2p/protocol/kademlia/impl/kad_backend.hpp>

namespace libp2p::protocol::kademlia {

  class KadSessionHost : public KadBackend {
   public:
    virtual void onMessage(connection::Stream *from, struct Message &&msg) = 0;

    virtual void onCompleted(connection::Stream *from,
                             outcome::result<void> res) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_SESSION_HOST_HPP
