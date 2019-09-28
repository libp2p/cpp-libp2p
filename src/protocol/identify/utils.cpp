/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/identify/utils.hpp>

#include <libp2p/multi/multiaddress.hpp>

namespace libp2p::protocol::detail {
  std::tuple<std::string, std::string> getPeerIdentity(
      const std::shared_ptr<libp2p::connection::Stream> &stream) {
    std::string id = "unknown";
    std::string addr = "unknown";
    if (auto id_res = stream->remotePeerId()) {
      id = id_res.value().toBase58();
    }
    if (auto addr_res = stream->remoteMultiaddr()) {
      addr = addr_res.value().getStringAddress();
    }
    return {std::move(id), std::move(addr)};
  }

  std::vector<peer::PeerInfo> getActivePeers(
      Host &host, network::ConnectionManager &conn_manager) {
    std::vector<peer::PeerInfo> active_peers;
    std::unordered_set<peer::PeerId> active_peers_ids;

    for (const auto &conn : conn_manager.getConnections()) {
      active_peers_ids.insert(conn->remotePeer().value());
    }

    auto &peer_repo = host.getPeerRepository();
    active_peers.reserve(active_peers_ids.size());
    for (const auto &peer_id : active_peers_ids) {
      active_peers.push_back(peer_repo.getPeerInfo(peer_id));
    }

    return active_peers;
  }

  void streamToEachConnectedPeer(Host &host,
                                 network::ConnectionManager &conn_manager,
                                 const peer::Protocol &protocol,
                                 const Host::StreamResultHandler &handler) {
    for (const auto &peer : getActivePeers(host, conn_manager)) {
      host.newStream(peer, protocol, handler);
    }
  }
}  // namespace libp2p::protocol::detail
