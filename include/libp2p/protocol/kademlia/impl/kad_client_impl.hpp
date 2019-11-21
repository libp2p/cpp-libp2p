/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_CLIENT_IMPL_HPP
#define LIBP2P_KAD_CLIENT_IMPL_HPP

#include <libp2p/protocol/kademlia/kad_client.hpp>
#include <libp2p/protocol/kademlia/kad_client_session.hpp>

namespace libp2p::protocol::kademlia {

  class KadClientImpl : public KadClient {
   public:
    ~KadClientImpl() override;

    //virtual void findPeerSingle(const Key &p, const peer::PeerId &id,
    //                            PeerInfosResultFunc f) override;


  };
}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_CLIENT_IMPL_HPP
