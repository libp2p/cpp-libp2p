/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_ID_UTILS_HPP
#define LIBP2P_ID_UTILS_HPP

#include <functional>
#include <memory>
#include <string>
#include <tuple>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/peer/protocol.hpp>

namespace libp2p::protocol::detail {
  /**
   * Get a tuple of stringified <PeerId, Multiaddress> of the peer the (\param
   * stream) is connected to
   */
  std::tuple<std::string, std::string> getPeerIdentity(
      const std::shared_ptr<libp2p::connection::Stream> &stream);

  /**
   * Get collection of peers, to which we have at least one active connection
   * @param host of this node
   * @param conn_manager of this node
   * @return PeerInfo-s
   */
  std::vector<peer::PeerInfo> getActivePeers(
      Host &host, network::ConnectionManager &conn_manager);

  // TODO(akvinikym) 15.07.19 PRE-252: when a broadcast will being implemented,
  // that method should be considered as a main part of it
  /**
   * Open a stream to each peer this host is connected to and execute a provided
   * function
   * @param host of this node
   * @param conn_manager of this node
   * @param protocol to open stream on
   * @param handler to be executed
   */
  void streamToEachConnectedPeer(Host &host,
                                 network::ConnectionManager &conn_manager,
                                 const peer::Protocol &protocol,
                                 const Host::StreamResultHandler &handler);
}  // namespace libp2p::protocol::detail

#endif  // LIBP2P_UTILS_HPP
