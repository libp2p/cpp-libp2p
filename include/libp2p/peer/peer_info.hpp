/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PEER_INFO_HPP
#define LIBP2P_PEER_INFO_HPP

#include <vector>

#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/common/logger.hpp>

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
  };

}  // namespace libp2p::peer

namespace std {
  template <>
  struct hash<libp2p::peer::PeerInfo> {
    size_t operator()(const libp2p::peer::PeerInfo &peer_info) const {
      return std::hash<libp2p::peer::PeerId>()(peer_info.id);
    }
  };
}  // namespace std

#endif  // LIBP2P_PEER_INFO_HPP
