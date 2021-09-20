/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LISTENER_MOCK_HPP
#define LIBP2P_LISTENER_MOCK_HPP

#include <libp2p/network/listener_manager.hpp>

#include <gmock/gmock.h>

namespace libp2p::network {
  struct ListenerMock : public ListenerManager {
    ~ListenerMock() override = default;

    MOCK_CONST_METHOD0(isStarted, bool());

    MOCK_METHOD0(start, void());

    MOCK_METHOD0(stop, void());

    MOCK_METHOD1(closeListener,
                 outcome::result<void>(const multi::Multiaddress &));

    MOCK_METHOD1(listen, outcome::result<void>(const multi::Multiaddress &));

    MOCK_CONST_METHOD0(getListenAddresses, std::vector<multi::Multiaddress>());

    MOCK_CONST_METHOD0(getListenAddressesInterfaces,
                       std::vector<multi::Multiaddress>());

    MOCK_METHOD1(handleProtocol, void(std::shared_ptr<protocol::BaseProtocol>));

    MOCK_METHOD2(setProtocolHandler,
                 void(const peer::Protocol &, StreamResultFunc));

    MOCK_METHOD3(setProtocolHandler,
                 void(const peer::Protocol &, StreamResultFunc,
                      Router::ProtoPredicate));

    MOCK_METHOD1(removeListener,
                 outcome::result<void>(const multi::Multiaddress &));

    MOCK_METHOD0(getRouter, Router &());

    MOCK_METHOD1(onConnection, void(
        outcome::result<std::shared_ptr<connection::CapableConnection>>));
  };
}  // namespace libp2p::network

#endif  // LIBP2P_LISTENER_MOCK_HPP
