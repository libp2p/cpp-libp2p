/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_ROUTER_MOCK_HPP
#define LIBP2P_ROUTER_MOCK_HPP

#include <libp2p/network/router.hpp>

#include <gmock/gmock.h>

namespace libp2p::network {
  struct RouterMock : public Router {
    ~RouterMock() override = default;

    MOCK_METHOD2(setProtocolHandler,
                 void(const peer::Protocol &, const ProtoHandler &));

    MOCK_METHOD3(setProtocolHandler,
                 void(const peer::Protocol &, const ProtoHandler &,
                      const ProtoPredicate &));

    MOCK_CONST_METHOD0(getSupportedProtocols, std::vector<peer::Protocol>());

    MOCK_METHOD1(removeProtocolHandlers, void(const peer::Protocol &));

    MOCK_METHOD0(removeAll, void());

    MOCK_METHOD2(handle,
                 outcome::result<void>(const peer::Protocol &,
                                       std::shared_ptr<connection::Stream>));
  };
}  // namespace libp2p::network

#endif  // LIBP2P_ROUTER_MOCK_HPP
