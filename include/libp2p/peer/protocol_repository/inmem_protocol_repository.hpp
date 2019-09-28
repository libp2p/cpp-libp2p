/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INMEM_PROTOCOL_REPOSITORY_HPP
#define LIBP2P_INMEM_PROTOCOL_REPOSITORY_HPP

#include <set>
#include <unordered_map>

#include <libp2p/peer/protocol_repository.hpp>

namespace libp2p::peer {

  /**
   * @brief In-memory implementation of Protocol repository. For each peer
   * stores ordered set of protocols.
   */
  class InmemProtocolRepository : public ProtocolRepository {
   public:
    ~InmemProtocolRepository() override = default;

    outcome::result<void> addProtocols(const PeerId &p,
                                       gsl::span<const Protocol> ms) override;

    outcome::result<void> removeProtocols(
        const PeerId &p, gsl::span<const Protocol> ms) override;

    outcome::result<std::vector<Protocol>> getProtocols(
        const PeerId &p) const override;

    outcome::result<std::vector<Protocol>> supportsProtocols(
        const PeerId &p, const std::set<Protocol> &protocols) const override;

    void clear(const PeerId &p) override;

    void collectGarbage() override;

    std::unordered_set<PeerId> getPeers() const override;

   private:
    using set = std::set<Protocol>;
    using set_ptr = std::shared_ptr<set>;

    outcome::result<set_ptr> getProtocolSet(const PeerId &p) const;
    set_ptr getOrAllocateProtocolSet(const PeerId &p);

    std::unordered_map<PeerId, set_ptr> db_;
  };

}  // namespace libp2p::peer

#endif  // LIBP2P_INMEM_PROTOCOL_REPOSITORY_HPP
