/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_OBSERVED_ADDRESSES_HPP
#define LIBP2P_OBSERVED_ADDRESSES_HPP

#include <chrono>
#include <unordered_map>
#include <vector>

#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/address_repository.hpp>

namespace libp2p::protocol {

  /**
   * Smart storage of mappings of our "official" listen addresses to the ones,
   * actually observed by other peers; this is needed, for example, if we use
   * NAT and want to understand, by which addresses we really are available
   */
  class ObservedAddresses {
    using Clock = std::chrono::steady_clock;
    using Milliseconds = std::chrono::milliseconds;

   public:
    /**
     * Get a set of addresses, which were observed by other peers, when they
     * tried to connect to the given (\param address)
     * @param address, for which the mapping is to be extracted
     * @return set of addresses
     */
    std::vector<multi::Multiaddress> getAddressesFor(
        const multi::Multiaddress &address) const;

    /**
     * Get all addresses, which were observed by other peers
     * @return the addresses
     */
    std::vector<multi::Multiaddress> getAllAddresses() const;

    /**
     * Add an address, which was observed by another peer
     * @param observed - the observed address itself
     * @param local - address, which the remote peer thought it connects to
     * @param observer - address of the remote peer, which observed the (\param
     * observed) address
     * @param is_initiator - was the remote peer an initiator of the connection?
     */
    void add(multi::Multiaddress observed, multi::Multiaddress local,
             const multi::Multiaddress &observer, bool is_initiator);

    /**
     * Get rid of expired addresses; should be called from time to time
     */
    void collectGarbage();

   private:
    /// address is to be seen by at least that number of times to become
    /// activated
    static constexpr uint8_t kActivationThresh = 4;

    struct Observation {
      Clock::time_point seen_time;
      bool observer_is_initiator{};
    };

    struct ObservedAddress {
      multi::Multiaddress address;
      std::unordered_map<multi::Multiaddress, Observation> seen_by;
      Clock::time_point last_seen;
      Milliseconds ttl = peer::ttl::kOwnObserved;
    };

    std::unordered_map<multi::Multiaddress, std::vector<ObservedAddress>>
        observed_addresses_;

    /**
     * Check if the address is activated: it was observed by a number of
     * different peers in some period of time
     * @param address to be checked
     * @param now - current time
     * @return true, if it is activated, false otherwise
     */
    bool addressIsActivated(const ObservedAddress &address,
                            Clock::time_point now) const;

    /**
     * Get "general" address of the provided (\param addr) to avoid issues, when
     * one peer under one IP address gets different ports because of NAT
     * @param addr, from which an observer group is to be extracted
     * @return the group
     */
    multi::Multiaddress observerGroup(const multi::Multiaddress &addr) const;
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_OBSERVED_ADDRESSES_HPP
