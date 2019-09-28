/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TRANSPORT_MANAGER_IMPL_HPP
#define LIBP2P_TRANSPORT_MANAGER_IMPL_HPP

#include <vector>

#include <libp2p/network/transport_manager.hpp>

namespace libp2p::network {
  class TransportManagerImpl : public TransportManager {
   public:
    /**
     * Initialize a transport manager from a collection of transports
     * @param transports, which this manager is going to support
     */
    explicit TransportManagerImpl(std::vector<TransportSPtr> transports);

    ~TransportManagerImpl() override = default;

    gsl::span<const TransportSPtr> getAll() const override;

    void clear() override;

    TransportSPtr findBest(const multi::Multiaddress &ma) override;

   private:
    std::vector<TransportSPtr> transports_;
  };
}  // namespace libp2p::network

#endif  // LIBP2P_TRANSPORT_MANAGER_IMPL_HPP
