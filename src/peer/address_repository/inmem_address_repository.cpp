/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/address_repository/inmem_address_repository.hpp>

#include <libp2p/peer/errors.hpp>

namespace libp2p::peer {

  outcome::result<void> InmemAddressRepository::addAddresses(
      const PeerId &p, gsl::span<const multi::Multiaddress> ma,
      AddressRepository::Milliseconds ttl) {
    auto peer_it = db_.emplace(p, std::make_shared<ttlmap>()).first;
    auto &addresses = *peer_it->second;

    auto expires_at = Clock::now() + ttl;
    for (const auto &m : ma) {
      addresses.emplace(m, expires_at);
      signal_added_(p, m);
    }

    return outcome::success();
  }

  outcome::result<void> InmemAddressRepository::upsertAddresses(
      const PeerId &p, gsl::span<const multi::Multiaddress> ma,
      AddressRepository::Milliseconds ttl) {
    auto peer_it = db_.emplace(p, std::make_shared<ttlmap>()).first;
    auto &addresses = *peer_it->second;

    auto expires_at = Clock::now() + ttl;
    for (auto &m : ma) {
      auto [addr_it, added] = addresses.emplace(m, expires_at);
      if (added) {
        signal_added_(p, m);
      } else {
        addr_it->second = expires_at;
      }
    }

    return outcome::success();
  }

  outcome::result<void> InmemAddressRepository::updateAddresses(
      const PeerId &p, Milliseconds ttl) {
    auto peer_it = db_.find(p);
    if (peer_it == db_.end()) {
      return PeerError::NOT_FOUND;
    }
    auto &addresses = *peer_it->second;

    auto expires_at = Clock::now() + ttl;
    std::for_each(addresses.begin(), addresses.end(),
                  [expires_at](auto &item) { item.second = expires_at; });

    return outcome::success();
  }  // namespace libp2p::peer

  outcome::result<std::vector<multi::Multiaddress>>
  InmemAddressRepository::getAddresses(const PeerId &p) const {
    auto peer_it = db_.find(p);
    if (peer_it == db_.end()) {
      return PeerError::NOT_FOUND;
    }
    auto &addresses = *peer_it->second;

    std::vector<multi::Multiaddress> ma;
    ma.reserve(addresses.size());
    std::transform(addresses.begin(), addresses.end(), std::back_inserter(ma),
                   [](auto &item) { return item.first; });
    return ma;
  }

  void InmemAddressRepository::clear(const PeerId &p) {
    auto it = db_.find(p);
    if (it != db_.end()) {
      for (const auto &item : *it->second) {
        signal_removed_(p, item.first);
      }
      it->second->clear();
    }
  }

  void InmemAddressRepository::collectGarbage() {
    auto now = Clock::now();
    auto peer = db_.begin();
    auto peer_end = db_.end();

    // for each peer
    while (peer != peer_end) {
      auto &&maptr = peer->second;

      // remove all expired addresses
      auto ma = maptr->begin();
      auto ma_end = maptr->end();
      while (ma != ma_end) {
        if (now >= ma->second) {
          signal_removed_(peer->first, ma->first);
          // erase returns element next to deleted
          ma = maptr->erase(ma);
        } else {
          ++ma;
        }
      }

      // peer has no more addresses
      if (peer->second->empty()) {
        // erase returns element next to deleted
        peer = db_.erase(peer);
      } else {
        ++peer;
      }
    }
  }

  std::unordered_set<PeerId> InmemAddressRepository::getPeers() const {
    std::unordered_set<PeerId> peers;
    for (const auto &it : db_) {
      peers.insert(it.first);
    }

    return peers;
  }

}  // namespace libp2p::peer
