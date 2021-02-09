/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_ADDRESS_REPOSITORY_HPP
#define LIBP2P_ADDRESS_REPOSITORY_HPP

#include <chrono>
#include <unordered_set>
#include <vector>

#include <boost/signals2.hpp>
#include <gsl/span>
#include <libp2p/basic/garbage_collectable.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace libp2p::peer {

  constexpr std::string_view kBootstrapAddress = "/dnsaddr/bootstrap.libp2p.io";

  namespace ttl {

    /// permanent addresses, for example, bootstrap nodes
    constexpr const auto kPermanent = std::chrono::milliseconds::max();

    /// standard expiration time of the addresses
    constexpr const auto kAddress = std::chrono::hours(1);

    /// we have recently connected to the peer and pretty certain about the
    /// address we add
    constexpr const auto kRecentlyConnected = std::chrono::minutes(10);

    /// for our own external addresses, observed by other peers
    constexpr const auto kOwnObserved = std::chrono::minutes(10);

    /// invalidated addresses
    constexpr const auto kTransient = std::chrono::seconds(10);

    constexpr const auto kDay = std::chrono::hours(24);

  }  // namespace ttl

  /**
   * @brief Address Repository is a storage of multiaddresses for observed
   * peers.
   */
  class AddressRepository : public basic::GarbageCollectable {
   protected:
    using Milliseconds = std::chrono::milliseconds;

   public:
    using AddressCallback = void(const PeerId &, const multi::Multiaddress &);
    using BootstrapCallback = void(outcome::result<void>);

    ~AddressRepository() override = default;

    /**
     * @brief Populate repository with peer infos discovered through the default
     * libp2p bootstrap address
     * @param cb - callback to accept the number of discovered peers or an error
     * of any
     */
    virtual void bootstrap(std::function<BootstrapCallback> cb) {
      auto ma_res = multi::Multiaddress::create(kBootstrapAddress);
      if (ma_res.has_error()) {
        cb(ma_res.error());
        return;
      }
      bootstrap(ma_res.value(), cb);
    }

    /**
     * @brief Populate repository with peer infos discovered through the
     * specified bootstrap address
     * @param ma - bootstrap node multiaddress, address format is
     * "/dnsaddr/<hostname>"
     * @param cb - callback to accept the number of discovered peers or an error
     * if any
     */
    virtual void bootstrap(const multi::Multiaddress &ma,
                           std::function<BootstrapCallback> cb) = 0;

    /**
     * @brief Add addresses to a given peer {@param p}
     * @param p peer
     * @param ma set of multiaddresses
     * @param ttl time to live for inserted multiaddresses
     * @return true/false if address was added or not,
     *  error when no peer {@param p} has been found
     *
     * @note triggers #onAddressAdded for each address
     */
    virtual outcome::result<bool> addAddresses(
        const PeerId &p, gsl::span<const multi::Multiaddress> ma,
        Milliseconds ttl) = 0;

    /**
     * @brief Update existing addresses with new {@param ttl} or insert new
     * addresses with new {@param ttl}
     * @param p peer
     * @param ma set of addresses
     * @param ttl ttl
     * @return true/false if at least one new address was added or not,
     *  error when no peer {@param p} has been found
     *

     * @note triggers #onAddressAdded when any new addresses are inserted
     */
    virtual outcome::result<bool> upsertAddresses(
        const PeerId &p, gsl::span<const multi::Multiaddress> ma,
        Milliseconds ttl) = 0;

    /**
     * @brief Update all addresses of a given peer {@param p}
     * @param p peer
     * @param ttl time to live for update multiaddresses
     * @return error when no peer has been found
     */
    virtual outcome::result<void> updateAddresses(const PeerId &p,
                                                  Milliseconds ttl) = 0;

    /**
     * @brief Get all addresses associated with this Peer {@param p}. May
     * contain duplicates.
     * @param p peer
     * @return array of addresses, or error when no peer {@param p} has been
     * found
     */
    virtual outcome::result<std::vector<multi::Multiaddress>> getAddresses(
        const PeerId &p) const = 0;

    /**
     * @brief Clear all addresses of given Peer {@param p}. Does not evict peer
     * from the list of known peers up to the next garbage collection.
     * @param p peer
     *
     * @note triggers #onAddressRemoved for every removed address
     */
    virtual void clear(const PeerId &p) = 0;

    /**
     * @brief Returns set of peer ids known by this repository.
     * @return unordered set of peers
     */
    virtual std::unordered_set<PeerId> getPeers() const = 0;

    /**
     * @brief Attach slot to a signal 'onAddressAdded'. Is triggered whenever
     * any peer adds new address.
     * @param cb slot
     * @return connection for that slot (can be used for unsubscribing)
     */
    boost::signals2::connection onAddressAdded(
        const std::function<AddressCallback> &cb);

    /**
     * @brief Attach slot to a signal 'onAddressRemoved'. Is triggered whenever
     * any peer removes address - happens when address is removed manually or
     * automatically via garbage collection mechanism.
     * @param cb slot
     * @return connection for that slot (can be used for unsubscribing)
     */
    boost::signals2::connection onAddressRemoved(
        const std::function<AddressCallback> &cb);

   protected:
    // TODO(warchant): change signals to events + Bus PRE-254

    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    boost::signals2::signal<AddressCallback> signal_added_;
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    boost::signals2::signal<AddressCallback> signal_removed_;
  };

}  // namespace libp2p::peer

#endif  // LIBP2P_ADDRESS_REPOSITORY_HPP
