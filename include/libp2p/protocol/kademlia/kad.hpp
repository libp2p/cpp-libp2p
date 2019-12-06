/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_KAD_HPP
#define LIBP2P_KADEMLIA_KAD_HPP

#include <libp2p/network/network.hpp>
#include <libp2p/protocol/base_protocol.hpp>
#include <libp2p/protocol/kademlia/value_store_backend.hpp>
#include <libp2p/protocol/kademlia/config.hpp>

namespace libp2p::protocol::kademlia {

  /**
   * @class Kad
   *
   * Entrypoint to a kademlia network.
   */
  class Kad {
   public:
    virtual ~Kad() = default;

    virtual void start(bool start_server) = 0;

    /// Adds \param peer_info into address store and routing table.
    /// \param permanent true for bootstrap peers
    virtual void addPeer(peer::PeerInfo peer_info, bool permanent) = 0;

    struct FindPeerQueryResult {
      PeerInfoSet closer_peers{};
      boost::optional<peer::PeerInfo> peer{};
      bool success = false;
    };

    // TODO(artem) : subscriptions instead of callbacks

    using FindPeerQueryResultFunc =
        std::function<void(const peer::PeerId &peer, FindPeerQueryResult)>;

    virtual bool findPeer(const peer::PeerId &peer,
                          FindPeerQueryResultFunc f) = 0;

    virtual bool findPeer(const peer::PeerId &peer,
                          const PeerInfoSet &closer_peers,
                          FindPeerQueryResultFunc f) = 0;

    using PutValueResult = outcome::result<void>;
    using PutValueResultFunc = std::function<void(PutValueResult)>;

    using GetValueResult = outcome::result<Value>;
    using GetValueResultFunc = std::function<void(GetValueResult)>;

    /// PutValue adds value corresponding to given Key.
    virtual void putValue(const ContentAddress& key, Value value,
        PutValueResultFunc f) = 0;

    /// GetValue searches for the value corresponding to given Key.
    virtual void getValue(const ContentAddress& key, GetValueResultFunc f) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KADEMLIA_KAD_HPP
