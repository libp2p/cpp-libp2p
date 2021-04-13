/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "peer_set.hpp"

#include <algorithm>
#include <random>
#include <chrono>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/for_each.hpp>

namespace libp2p::protocol::gossip {

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
    if (n > 0 && !empty()) {
      ret.reserve(n > size() ? size() : n);
      std::mt19937 gen;
      gen.seed(std::chrono::system_clock::now().time_since_epoch().count());
      std::sample(peers_.begin(), peers_.end(), std::back_inserter(ret), n,
                  gen);
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
