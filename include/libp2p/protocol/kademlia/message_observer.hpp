/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_MESSAGEOBSERVER
#define LIBP2P_PROTOCOL_KADEMLIA_MESSAGEOBSERVER

#include <functional>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::protocol::kademlia {

class MessageObserver {
public:
		using FoundPeerInfoHandler =
		std::function<void(outcome::result<peer::PeerInfo>)>;

		virtual ~MessageObserver() = default;

		/// Searches for a peer with given @param ID, @returns PeerInfo
		// with relevant addresses.
		virtual outcome::result<void> onMessage(const peer::PeerId &peer_id,
		                                       FoundPeerInfoHandler handler) = 0;
};

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_MESSAGEOBSERVER
