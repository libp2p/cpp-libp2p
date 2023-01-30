/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/protocol_repository/inmem_protocol_repository.hpp>

#include <libp2p/peer/errors.hpp>

namespace libp2p::peer {

  outcome::result<void> InmemProtocolRepository::addProtocols(
      const PeerId &p, gsl::span<const ProtocolName> ms) {
    auto s = getOrAllocateProtocolSet(p);
    for (const auto &m : ms) {
      s->insert(m);
    }

    return outcome::success();
  }

  outcome::result<void> InmemProtocolRepository::removeProtocols(
      const PeerId &p, gsl::span<const ProtocolName> ms) {
    OUTCOME_TRY(s, getProtocolSet(p));

    for (const auto &m : ms) {
      s->erase(m);
    }

    return outcome::success();
  }

  outcome::result<std::vector<ProtocolName>> InmemProtocolRepository::getProtocols(
      const PeerId &p) const {
    OUTCOME_TRY(s, getProtocolSet(p));
    return std::vector<ProtocolName>(s->begin(), s->end());
  }

  outcome::result<std::vector<ProtocolName>>
  InmemProtocolRepository::supportsProtocols(
      const PeerId &p, const std::set<ProtocolName> &protocols) const {
    OUTCOME_TRY(s, getProtocolSet(p));

    size_t size = std::min(protocols.size(), s->size());
    std::vector<ProtocolName> ret;
    ret.reserve(size);

    std::set_intersection(protocols.begin(), protocols.end(), s->begin(),
                          s->end(), std::back_inserter(ret));
    return ret;
  }

  void InmemProtocolRepository::clear(const PeerId &p) {
    auto r = getProtocolSet(p);
    if (r) {
      r.value()->clear();
    }
  }

  outcome::result<InmemProtocolRepository::set_ptr>
  InmemProtocolRepository::getProtocolSet(const PeerId &p) const {
    auto it = db_.find(p);
    if (it == db_.end()) {
      return PeerError::NOT_FOUND;
    }

    return it->second;
  }

  InmemProtocolRepository::set_ptr
  InmemProtocolRepository::getOrAllocateProtocolSet(const PeerId &p) {
    auto it = db_.find(p);
    if (it == db_.end()) {
      set_ptr m = std::make_shared<set>();
      db_[p] = m;
      return m;
    }

    return it->second;
  }

  void InmemProtocolRepository::collectGarbage() {
    auto peer = db_.begin();
    auto peerend = db_.end();
    while (peer != peerend) {
      if (peer->second->empty()) {
        // erase returns element next to deleted
        peer = db_.erase(peer);
      } else {
        ++peer;
      }
    }
  }

  std::unordered_set<PeerId> InmemProtocolRepository::getPeers() const {
    std::unordered_set<PeerId> peers;
    for (const auto &it : db_) {
      peers.insert(it.first);
    }

    return peers;
  }

}  // namespace libp2p::peer
