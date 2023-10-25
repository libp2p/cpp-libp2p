/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/protocol/base_protocol.hpp>

#include <gmock/gmock.h>

namespace libp2p::protocol {

  struct BaseProtocolMock : public BaseProtocol {
    ~BaseProtocolMock() override = default;

    MOCK_CONST_METHOD0(getProtocolId, peer::ProtocolName());
    MOCK_METHOD1(handle, void(StreamAndProtocol));
  };

}  // namespace libp2p::protocol
