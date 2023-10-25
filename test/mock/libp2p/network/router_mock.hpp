/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/network/router.hpp>

#include <gmock/gmock.h>

namespace libp2p::network {
  struct RouterMock : public Router {
    ~RouterMock() override = default;

    MOCK_METHOD3(setProtocolHandler,
                 void(StreamProtocols, StreamAndProtocolCb, ProtocolPredicate));

    MOCK_CONST_METHOD0(getSupportedProtocols,
                       std::vector<peer::ProtocolName>());

    MOCK_METHOD1(removeProtocolHandlers, void(const peer::ProtocolName &));

    MOCK_METHOD0(removeAll, void());

    MOCK_METHOD2(handle,
                 outcome::result<void>(const peer::ProtocolName &,
                                       std::shared_ptr<connection::Stream>));
  };
}  // namespace libp2p::network
