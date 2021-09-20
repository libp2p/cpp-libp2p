/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TRANSPORT_MANAGER_MOCK_HPP
#define LIBP2P_TRANSPORT_MANAGER_MOCK_HPP

#include <libp2p/network/transport_manager.hpp>

#include <gmock/gmock.h>

namespace libp2p::network {

  struct TransportManagerMock : public TransportManager {
    ~TransportManagerMock() override = default;

    MOCK_METHOD1(add, void(TransportSPtr));
    MOCK_METHOD1(add, void(gsl::span<const TransportSPtr>));
    MOCK_CONST_METHOD0(getAll, gsl::span<const TransportSPtr>());
    MOCK_METHOD0(clear, void());
    MOCK_METHOD1(findBest, TransportSPtr(const multi::Multiaddress &));
  };

}  // namespace libp2p::network

#endif  // LIBP2P_TRANSPORT_MANAGER_MOCK_HPP
