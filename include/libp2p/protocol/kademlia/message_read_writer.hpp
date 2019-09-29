/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_MESSAGE_READ_WRITER_HPP
#define LIBP2P_KAD_MESSAGE_READ_WRITER_HPP

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  /// An interface, which sends actual messages to other peers.
  struct MessageReadWriter {
    virtual ~MessageReadWriter() = default;

    using PeerInfos = std::vector<peer::PeerInfo>;
    using PeerInfosResult = outcome::result<PeerInfos>;
    using PeerInfosResultFunc = std::function<void(PeerInfosResult)>;

    virtual void findPeerSingle(const Key &p, const peer::PeerId &id,
                                PeerInfosResultFunc f) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_MESSAGE_READ_WRITER_HPP
