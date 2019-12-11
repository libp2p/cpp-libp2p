/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

#include "peers.hpp"

namespace libp2p::protocol::gossip {

  PeerContext::Ptr PeerSet::find(const PeerId &id) const {
    auto it = peers_.find(id);
    if (it == peers_.end()) {
      return PeerContext::Ptr{};
    }
    return it->ptr;
  }

  bool PeerSet::insert(PeerContext::Ptr ctx) {
    if (!ctx || peers_.find(ctx) != peers_.end()) {
      return false;
    }
    peer_ids_.push_back(ctx->peer_id);
    peers_.emplace(std::move(ctx));
    return true;
  }

  bool PeerSet::erase(const PeerId &id) {
    auto it = peers_.find(id);
    if (it == peers_.end()) {
      return false;
    }
    peers_.erase(it);

    // linear. But our peer sets are very limited in size, by design

    auto v_it = std::find(peer_ids_.begin(), peer_ids_.end(), id);
    assert(v_it != peer_ids_.end());
    peer_ids_.erase(v_it);

    return true;
  }

  void PeerSet::clear() {
    peers_.clear();
    peer_ids_.clear();
  }

  bool PeerSet::empty() const {
    return peer_ids_.empty();
  }

  size_t PeerSet::size() const {
    return peer_ids_.size();
  }

  std::vector<PeerContext::Ptr> PeerSet::select(size_t n,
                                                UniformRandomGen &gen) const {
    std::vector<PeerContext::Ptr> ret;
    if (n > 0) {
      size_t sz = size();
      ret.reserve(n > sz ? sz : n);
      if (n >= sz) {
        // return all peers as vector
        for (auto& p : peers_) {
          ret.push_back(p.ptr);
        }
      } else if (n == 1) {
        // no need to shuffle, return 1 random peer
        auto ptr = find(peer_ids_[gen(sz)]);
        assert(ptr);
        ret.push_back(ptr);
      } else {
        // shuffle and return n first peers
        for (size_t i=0; i<n-1; ++i) {
          auto r = i + gen(n - i);
          if (r != i) {
            std::swap(peer_ids_[i], peer_ids_[r]);
          }
          auto ptr = find(peer_ids_[i]);
          assert(ptr);
          ret.push_back(ptr);
        }
      }
    }
    return ret;
  }

}  // namespace libp2p::protocol::gossip
