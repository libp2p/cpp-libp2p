/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/address_repository/inmem_address_repository.hpp>

#include <libp2p/peer/errors.hpp>

namespace libp2p::peer {

  InmemAddressRepository::InmemAddressRepository(
      std::shared_ptr<network::DnsaddrResolver> dnsaddr_resolver)
      : dnsaddr_resolver_{std::move(dnsaddr_resolver)} {
    BOOST_ASSERT(dnsaddr_resolver_);
  }

  void InmemAddressRepository::bootstrap(const multi::Multiaddress &ma,
                                         std::function<BootstrapCallback> cb) {
    if (not isNewDnsAddr(ma)) {
      return;  // just skip silently circular references among dnsaddrs
    }
    dnsaddr_resolver_->load(
        ma,
        [self{shared_from_this()}, callback{std::move(cb)}](
            outcome::result<std::vector<multi::Multiaddress>> result) {
          if (result.has_error()) {
            callback(result.error());
            return;
          }
          auto &&addrs = result.value();
          for (const auto &addr : addrs) {
            if (addr.hasProtocol(multi::Protocol::Code::DNS_ADDR)) {
              self->bootstrap(addr, callback);
            } else {
              auto peer_id_str = addr.getPeerId();
              if (peer_id_str) {
                auto peer_id = peer::PeerId::fromBase58(peer_id_str.get());
                if (peer_id.has_value()) {
                  std::vector addr_vec = {addr};
                  if (auto added = self->addAddresses(
                          peer_id.value(), addr_vec, kDefaultTtl);
                      added.has_error()) {
                    callback(added.error());
                  } else {
                    callback(outcome::success());
                  }
                } else {
                  callback(peer_id.error());
                  return;
                }
              }
            }
          }
        });
  }

  bool InmemAddressRepository::isNewDnsAddr(const multi::Multiaddress &ma) {
    if (resolved_dns_addrs_.end() == resolved_dns_addrs_.find(ma)) {
      resolved_dns_addrs_.insert(ma);
      return true;
    }
    return false;
  }

  Clock::time_point InmemAddressRepository::calculateExpirationTime(
      const Milliseconds &ttl) const {
    if (ttl >= std::chrono::milliseconds::max() / 2) {
      // Handle "permanent" or extremely long TTL values safely
      return Clock::time_point::max();
    }
    return Clock::now() + ttl;
  }

  outcome::result<bool> InmemAddressRepository::addAddresses(
      const PeerId &p,
      std::span<const multi::Multiaddress> ma,
      AddressRepository::Milliseconds ttl) {
    bool added = false;
    auto &addresses = db_[p];

    auto expires_at = calculateExpirationTime(ttl);

    for (const auto &m : ma) {
      if (addresses.expires.emplace(m, expires_at).second) {
        addresses.order.emplace_back(m);
        signal_added_(p, m);
        added = true;
      }
    }

    return added;
  }

  outcome::result<bool> InmemAddressRepository::upsertAddresses(
      const PeerId &p,
      std::span<const multi::Multiaddress> ma,
      AddressRepository::Milliseconds ttl) {
    bool added = false;
    auto &addresses = db_[p];

    auto expires_at = calculateExpirationTime(ttl);

    for (const auto &m : ma) {
      auto [addr_it, emplaced] = addresses.expires.emplace(m, expires_at);
      if (emplaced) {
        addresses.order.emplace_back(m);
        signal_added_(p, m);
        added = true;
      } else {
        addr_it->second = expires_at;
      }
    }

    return added;
  }

  outcome::result<void> InmemAddressRepository::updateAddresses(
      const PeerId &p, Milliseconds ttl) {
    auto peer_it = db_.find(p);
    if (peer_it == db_.end()) {
      return PeerError::NOT_FOUND;
    }
    auto &addresses = peer_it->second;

    auto expires_at = calculateExpirationTime(ttl);

    for (auto &item : addresses.expires) {
      item.second = expires_at;
    }

    return outcome::success();
  }

  void InmemAddressRepository::dialFailed(const PeerId &peer_id,
                                          const Multiaddress &addr) {
    auto peer_it = db_.find(peer_id);
    if (peer_it == db_.end()) {
      return;
    }
    auto &peer = peer_it->second;
    if (not peer.expires.contains(addr)) {
      return;
    }
    if (not peer.eraseOrder(addr)) {
      return;
    }
    peer.order.emplace_back(addr);
  }

  outcome::result<std::vector<multi::Multiaddress>>
  InmemAddressRepository::getAddresses(const PeerId &p) const {
    auto peer_it = db_.find(p);
    if (peer_it == db_.end()) {
      return PeerError::NOT_FOUND;
    }
    return peer_it->second.order;
  }

  void InmemAddressRepository::clear(const PeerId &p) {
    auto it = db_.find(p);
    if (it != db_.end()) {
      for (const auto &item : it->second.expires) {
        signal_removed_(p, item.first);
      }
      it->second = {};
    }
  }

  void InmemAddressRepository::collectGarbage() {
    auto now = Clock::now();
    auto peer = db_.begin();
    auto peer_end = db_.end();

    // for each peer
    while (peer != peer_end) {
      auto &maptr = peer->second;

      // remove all expired addresses
      auto ma = maptr.expires.begin();
      auto ma_end = maptr.expires.end();
      while (ma != ma_end) {
        if (now >= ma->second) {
          signal_removed_(peer->first, ma->first);
          maptr.eraseOrder(ma->first);
          // erase returns element next to deleted
          ma = maptr.expires.erase(ma);
        } else {
          ++ma;
        }
      }

      // peer has no more addresses
      if (peer->second.expires.empty()) {
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

  bool InmemAddressRepository::Peer::eraseOrder(const Multiaddress &addr) {
    auto it = std::ranges::find(order, addr);
    if (it == order.end()) {
      return false;
    }
    order.erase(it);
    return true;
  }
}  // namespace libp2p::peer
