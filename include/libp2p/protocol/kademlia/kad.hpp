/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_KAD_HPP
#define LIBP2P_KADEMLIA_KAD_HPP

#include <libp2p/network/network.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/base_protocol.hpp>
//#include <libp2p/protocol/kademlia/value_store.hpp>
//#include <libp2p/routing/content_routing.hpp>
//#include <libp2p/routing/peer_routing.hpp>

namespace libp2p::protocol::kademlia {

  enum class Error {
    SUCCESS = 0,
    NO_PEERS = 1,
    MESSAGE_PARSE_ERROR = 2,
    MESSAGE_SERIALIZE_ERROR = 3,
    UNEXPECTED_MESSAGE_TYPE = 4,
    STREAM_RESET = 5
  };

  /**
   * @class Kad
   *
   * Entrypoint to a kademlia network.
   */
  class Kad : public protocol::BaseProtocol {
   public:
    ~Kad() override = default;

    virtual void start(bool start_server) = 0;

    // permanent==true for bootstrap peers
    virtual void addPeer(peer::PeerInfo peer_info, bool permanent) = 0;

    struct FindPeerQueryResult {
      PeerInfoSet closer_peers{};
      boost::optional<peer::PeerInfo> peer{};
      bool success = false;
    };

    using FindPeerQueryResultFunc = std::function<void(const peer::PeerId& peer, FindPeerQueryResult)>;

    virtual bool findPeer(const peer::PeerId& peer, FindPeerQueryResultFunc f) = 0;

    virtual bool findPeer(const peer::PeerId& peer, const PeerInfoSet& closer_peers, FindPeerQueryResultFunc f) = 0;
  };

}  // namespace libp2p::protocol::kademlia

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia, Error);

#endif  // LIBP2P_KADEMLIA_KAD_HPP
