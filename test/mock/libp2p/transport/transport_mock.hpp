/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TRANSPORT_MOCK_HPP
#define LIBP2P_TRANSPORT_MOCK_HPP

#include <libp2p/transport/transport_adaptor.hpp>

#include <gmock/gmock.h>

namespace libp2p::transport {
  class TransportMock : public TransportAdaptor {
   public:
    ~TransportMock() override = default;

    MOCK_CONST_METHOD0(getProtocolId, peer::Protocol());

    MOCK_METHOD3(dial,
                 void(const peer::PeerId &, multi::Multiaddress, HandlerFunc));
    MOCK_METHOD4(dial,
                 void(const peer::PeerId &, multi::Multiaddress, HandlerFunc,
                      std::chrono::milliseconds));

    MOCK_METHOD1(
        createListener,
        std::shared_ptr<TransportListener>(TransportListener::HandlerFunc));

    MOCK_CONST_METHOD1(canDial, bool(const multi::Multiaddress &));
  };
}  // namespace libp2p::transport

#endif  // LIBP2P_TRANSPORT_MOCK_HPP
