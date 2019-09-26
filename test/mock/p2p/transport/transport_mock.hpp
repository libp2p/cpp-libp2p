/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_MOCK_HPP
#define KAGOME_TRANSPORT_MOCK_HPP

#include <gmock/gmock.h>
#include "p2p/transport/transport_adaptor.hpp"

namespace libp2p::transport {
  class TransportMock : public TransportAdaptor {
   public:
    ~TransportMock() override = default;

    MOCK_CONST_METHOD0(getProtocolId, peer::Protocol());

    MOCK_METHOD3(dial,
                 void(const peer::PeerId &, multi::Multiaddress, HandlerFunc));

    MOCK_METHOD1(
        createListener,
        std::shared_ptr<TransportListener>(TransportListener::HandlerFunc));

    MOCK_CONST_METHOD1(canDial, bool(const multi::Multiaddress &));
  };
}  // namespace libp2p::transport

#endif  // KAGOME_TRANSPORT_MOCK_HPP
