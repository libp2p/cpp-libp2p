/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/identify/observed_addresses.hpp>

#include <algorithm>

namespace libp2p::protocol {
  std::vector<multi::Multiaddress> ObservedAddresses::getAddressesFor(
      const multi::Multiaddress &address) const {
    std::vector<multi::Multiaddress> result;

    auto addr_entry_it = observed_addresses_.find(address);
    if (addr_entry_it == observed_addresses_.end()) {
      return result;
    }

    auto now = Clock::now();
    for (const auto &addr : addr_entry_it->second) {
      if (addressIsActivated(addr, now)) {
        result.push_back(addr.address);
      }
    }

    return result;
  }

  std::vector<multi::Multiaddress> ObservedAddresses::getAllAddresses() const {
    std::vector<multi::Multiaddress> result;

    for (const auto &it : observed_addresses_) {
      auto addresses = getAddressesFor(it.first);
      result.insert(result.end(), std::make_move_iterator(addresses.begin()),
                    std::make_move_iterator(addresses.end()));
    }

    return result;
  }

  void ObservedAddresses::add(multi::Multiaddress observed,
                              multi::Multiaddress local,
                              const multi::Multiaddress &observer,
                              bool is_initiator) {
    auto now = Clock::now();
    auto observer_group = observerGroup(observer);

    Observation observation{now, is_initiator};

    auto local_addr_entry = observed_addresses_.find(local);
    if (local_addr_entry == observed_addresses_.end()) {
      // this is the first time somebody was connecting to this local address
      local_addr_entry =
          observed_addresses_
              .emplace(std::make_pair(std::move(local),
                                      std::vector<ObservedAddress>()))
              .first;
    }

    auto &addresses = local_addr_entry->second;
    auto observed_addr_it =
        std::find_if(addresses.begin(), addresses.end(),
                     [&observed](const auto &observed_addr) {
                       return observed_addr.address == observed;
                     });
    if (observed_addr_it == addresses.end()) {
      // this observed address was not mapped to that local address before
      std::unordered_map<multi::Multiaddress, Observation> seen_by{
          std::make_pair(std::move(observer_group), observation)};
      addresses.push_back(
          ObservedAddress{std::move(observed), std::move(seen_by), now});
      return;
    }

    // update the address observation
    observed_addr_it->seen_by[observer_group] = observation;
    observed_addr_it->last_seen = now;
  }

  void ObservedAddresses::collectGarbage() {
    auto now = Clock::now();
    for (auto &addr_entry : observed_addresses_) {
      // firstly, remove the "outer" structures with observed addresses, which
      // were expired
      auto &addresses = addr_entry.second;
      addresses.erase(std::remove_if(addresses.begin(), addresses.end(),
                                     [now](const auto &observed_addr) {
                                       return now - observed_addr.last_seen
                                           > observed_addr.ttl;
                                     }),
                      addresses.end());

      // secondly, clear the "inner" structures with observation facts
      for (auto &addr : addr_entry.second) {
        auto it = addr.seen_by.begin();
        while (it != addr.seen_by.end()) {
          if (now - it->second.seen_time > addr.ttl * kActivationThresh) {
            it = addr.seen_by.erase(it);
          } else {
            ++it;
          }
        }
      }
    }
  }

  bool ObservedAddresses::addressIsActivated(const ObservedAddress &address,
                                             Clock::time_point now) const {
    return now - address.last_seen <= address.ttl
        && address.seen_by.size() >= kActivationThresh;
  }

  multi::Multiaddress ObservedAddresses::observerGroup(
      const multi::Multiaddress &addr) const {
    // for now, we only use the root part of the multiaddress; in most cases, it
    // is IP4, and this is the behaviour we want
    return addr.splitFirst().first;
  }
}  // namespace libp2p::protocol
