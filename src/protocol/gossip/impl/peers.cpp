/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/peers.hpp>

#include <algorithm>
#include <random>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/for_each.hpp>


namespace libp2p::protocol::gossip {

  bool operator<(const PeerContextPtr &ctx, const peer::PeerId &peer) {
    if (!ctx)
      return false;
    return less(ctx->peer_id, peer);
  }

  bool operator<(const peer::PeerId &peer, const PeerContextPtr &ctx) {
    if (!ctx)
      return false;
    return less(peer, ctx->peer_id);
  }

  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b) {
    if (!a || !b)
      return false;
    return less(a->peer_id, b->peer_id);
  }

  boost::optional<PeerContextPtr> PeerSet::find(
      const peer::PeerId &id) const {
    auto it = peers_.find(id);
    if (it == peers_.end()) {
      return boost::none;
    }
    return *it;
  }

  bool PeerSet::contains(const peer::PeerId &id) const {
    return peers_.count(id) != 0;
  }

  bool PeerSet::insert(PeerContextPtr ctx) {
    if (!ctx || peers_.find(ctx) != peers_.end()) {
      return false;
    }
    peers_.emplace(std::move(ctx));
    return true;
  }

  boost::optional<PeerContextPtr> PeerSet::erase(const peer::PeerId &id) {
    auto it = peers_.find(id);
    if (it == peers_.end()) {
      return boost::none;
    }
    boost::optional<PeerContextPtr> ret(*it);
    peers_.erase(it);
    return ret;
  }

  void PeerSet::clear() {
    peers_.clear();
  }

  bool PeerSet::empty() const {
    return peers_.empty();
  }

  size_t PeerSet::size() const {
    return peers_.size();
  }

  std::vector<PeerContextPtr> PeerSet::selectRandomPeers(size_t n) const {
    std::vector<PeerContextPtr> ret;
    if (n > 0) {
      ret.reserve(n > size() ? size() : n);
      std::sample(peers_.begin(), peers_.end(), std::back_inserter(ret), n,
                  std::mt19937{std::random_device{}()});
    }
    return ret;
  }

  void PeerSet::selectAll(const SelectCallback &callback) const {
    boost::for_each(peers_, callback);
  }

  void PeerSet::selectIf(const SelectCallback &callback,
                         const FilterCallback &filter) const {
    boost::for_each(peers_ | boost::adaptors::filtered(filter), callback);
  }

  void PeerSet::eraseIf(const FilterCallback &filter) {
    for (auto it = peers_.begin(); it != peers_.end();) {
      if (filter(*it)) {
        it = peers_.erase(it);
      } else {
        ++it;
      }
    }
  }

}  // namespace libp2p::protocol::gossip
