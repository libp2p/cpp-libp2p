/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_CLIENT_HPP
#define LIBP2P_KAD_CLIENT_HPP

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <memory>

namespace libp2p::protocol::kademlia {

  class KadClient : public std::enable_shared_from_this<KadClient> {
   public:
    using PeerInfos = std::vector<peer::PeerInfo>;
    using PeerInfosResult = outcome::result<PeerInfos>;
    using PeerInfosResultFunc = std::function<void(PeerInfosResult)>;

    ~KadClient() = default;

    //virtual void findPeerSingle(const Key &p, const peer::PeerId &id,
    //                            PeerInfosResultFunc f) = 0;

   private:

  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_CLIENT_HPP
