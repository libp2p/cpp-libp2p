/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/kademlia/content_id.hpp>
#include <libp2p/protocol/kademlia/content_value.hpp>

namespace libp2p::protocol::kademlia {

  using namespace std::chrono_literals;

  using peer::PeerId;
  using peer::PeerInfo;

  using Key = ContentId;
  using Value = ContentValue;
  using Time = std::chrono::milliseconds;
  using ValueAndTime = std::pair<Value, Time>;

  using FoundPeerInfoHandler = std::function<void(outcome::result<PeerInfo>)>;
  using FoundProvidersHandler =
      std::function<void(outcome::result<std::vector<PeerInfo>>)>;
  using FoundValueHandler = std::function<void(outcome::result<Value>)>;

}  // namespace libp2p::protocol::kademlia
