/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include <libp2p/log/logger.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace libp2p::peer {

  struct PeerInfo {
    PeerId id;
    std::vector<multi::Multiaddress> addresses;

    bool operator==(const PeerInfo &other) const {
      return id == other.id && addresses == other.addresses;
    }
    bool operator!=(const PeerInfo &other) const {
      return !(*this == other);
    }

    struct EqualByPeerId {
      bool operator()(const PeerInfo &lhs, const PeerInfo &rhs) const {
        return lhs.id == rhs.id;
      }
    };

    struct CompareByPeerId {
      bool operator()(const PeerInfo &lhs, const PeerInfo &rhs) const {
        return lhs.id.toVector() < rhs.id.toVector();
      }
    };
  };

}  // namespace libp2p::peer

namespace libp2p {
  using peer::PeerInfo;
}  // namespace libp2p

namespace std {
  template <>
  struct hash<libp2p::peer::PeerInfo> {
    size_t operator()(const libp2p::peer::PeerInfo &peer_info) const {
      return std::hash<libp2p::peer::PeerId>()(peer_info.id);
    }
  };
}  // namespace std
