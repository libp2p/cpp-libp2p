/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/network/impl/transport_manager_impl.hpp>

#include <algorithm>

namespace libp2p::network {
  TransportManagerImpl::TransportManagerImpl(
      std::vector<TransportSPtr> transports)
      : transports_{std::move(transports)} {
    BOOST_ASSERT_MSG(!transports_.empty(),
                     "TransportManagerImpl got 0 transports");
    BOOST_ASSERT(std::all_of(transports_.begin(), transports_.end(),
                             [](auto &&t) { return t != nullptr; }));
  }

  gsl::span<const TransportManagerImpl::TransportSPtr>
  TransportManagerImpl::getAll() const {
    return transports_;
  }

  void TransportManagerImpl::clear() {
    transports_.clear();
  }

  TransportManagerImpl::TransportSPtr TransportManagerImpl::findBest(
      const multi::Multiaddress &ma) {
    auto it = std::find_if(transports_.begin(), transports_.end(),
                           [&ma](const auto &t) { return t->canDial(ma); });
    if (it != transports_.end()) {
      return *it;
    }
    return nullptr;
  }
}  // namespace libp2p::network
