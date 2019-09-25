/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_INMEM_ADDRESS_REPOSITORY_HPP
#define KAGOME_INMEM_ADDRESS_REPOSITORY_HPP

#include <unordered_map>
#include <vector>

#include "peer/address_repository.hpp"

namespace libp2p::peer {

  /**
   * @brief Default clock that is used for TTLs. Steady clock guarantees that
   * for each invocation, time "continues to go forward".
   */
  using Clock = std::chrono::steady_clock;

  /**
   * @brief IN-memory implementation of Address repository.
   */
  class InmemAddressRepository : public AddressRepository {
   public:
    outcome::result<void> addAddresses(const PeerId &p,
                                       gsl::span<const multi::Multiaddress> ma,
                                       Milliseconds ttl) override;

    outcome::result<void> upsertAddresses(
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

    std::unordered_map<PeerId, ttlmap_ptr> db_;
  };

}  // namespace libp2p::peer

#endif  // KAGOME_INMEM_ADDRESS_REPOSITORY_HPP
