/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_PEER_SET_HPP
#define LIBP2P_PROTOCOL_GOSSIP_PEER_SET_HPP

#include <functional>
#include <set>

#include "peer_context.hpp"

namespace libp2p::protocol::gossip {

  /// Peer set for pub-sub protocols
  class PeerSet {
   public:
    /// Finds peer context by id
    boost::optional<PeerContextPtr> find(const peer::PeerId &id) const;

    /// Returns if the set contains such a peer
    bool contains(const peer::PeerId &id) const;

    /// Inserts peer context into set, returns false if already inserted
    bool insert(PeerContextPtr ctx);

    /// Removes peer context from set, returns erased item if found and erased
    boost::optional<PeerContextPtr> erase(const peer::PeerId &id);

    /// Clears all data
    void clear();

    /// Returns true iff size() == 0
    bool empty() const;

    /// Returns # of peers in list
    size_t size() const;

    /// Selects up to n random peers
    std::vector<PeerContextPtr> selectRandomPeers(size_t n) const;

    /// Callback for peer selection
    using SelectCallback = std::function<void(const PeerContextPtr &)>;

    /// Callback for peer filtering
    using FilterCallback = std::function<bool(const PeerContextPtr &)>;

    /// Selects all peers
    void selectAll(const SelectCallback &callback) const;

    /// Selects peers filtered by external criteria
    void selectIf(const SelectCallback &callback,
                  const FilterCallback &filter) const;

    /// Conditionally erases peers from the set
    void eraseIf(const FilterCallback &filter);

   private:
    std::set<PeerContextPtr, std::less<>> peers_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_PEER_SET_HPP
