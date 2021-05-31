/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INMEM_ADDRESS_REPOSITORY_HPP
#define LIBP2P_INMEM_ADDRESS_REPOSITORY_HPP

#include <libp2p/peer/address_repository.hpp>

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include <libp2p/network/dnsaddr_resolver.hpp>

namespace libp2p::peer {

  /**
   * @brief Default clock that is used for TTLs. Steady clock guarantees that
   * for each invocation, time "continues to go forward".
   */
  using Clock = std::chrono::steady_clock;

  /**
   * @brief IN-memory implementation of Address repository.
   */
  class InmemAddressRepository
      : public AddressRepository,
        public std::enable_shared_from_this<InmemAddressRepository> {
   public:
    static constexpr auto kDefaultTtl = std::chrono::milliseconds(1000);

    explicit InmemAddressRepository(
        std::shared_ptr<network::DnsaddrResolver> dnsaddr_resolver);

    void bootstrap(const multi::Multiaddress &ma,
                   std::function<BootstrapCallback> cb) override;

    outcome::result<bool> addAddresses(const PeerId &p,
                                       gsl::span<const multi::Multiaddress> ma,
                                       Milliseconds ttl) override;

    outcome::result<bool> upsertAddresses(
        const PeerId &p, gsl::span<const multi::Multiaddress> ma,
        Milliseconds ttl) override;

    outcome::result<void> updateAddresses(const PeerId &p,
                                          Milliseconds ttl) override;

    outcome::result<std::vector<multi::Multiaddress>> getAddresses(
        const PeerId &p) const override;

    void collectGarbage() override;

    void clear(const PeerId &p) override;

    std::unordered_set<PeerId> getPeers() const override;

   private:
    using ttlmap = std::unordered_map<multi::Multiaddress, Clock::time_point>;
    using ttlmap_ptr = std::shared_ptr<ttlmap>;
    using peer_db = std::unordered_map<PeerId, ttlmap_ptr>;

    bool isNewDnsAddr(const multi::Multiaddress &ma);

    peer_db::iterator findOrInsert(const PeerId &p);

    std::shared_ptr<network::DnsaddrResolver> dnsaddr_resolver_;
    peer_db db_;
    std::set<multi::Multiaddress> resolved_dns_addrs_;
  };

}  // namespace libp2p::peer

#endif  // LIBP2P_INMEM_ADDRESS_REPOSITORY_HPP
