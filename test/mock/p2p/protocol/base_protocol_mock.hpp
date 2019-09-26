/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_PROTOCOL_MOCK_HPP
#define KAGOME_BASE_PROTOCOL_MOCK_HPP

#include <gmock/gmock.h>
#include "p2p/protocol/base_protocol.hpp"

namespace libp2p::protocol {

  struct BaseProtocolMock : public BaseProtocol {
    ~BaseProtocolMock() override = default;

    MOCK_CONST_METHOD0(getProtocolId, peer::Protocol());
    MOCK_METHOD1(handle, void(StreamResult));
  };

}  // namespace libp2p::protocol

#endif  // KAGOME_BASE_PROTOCOL_MOCK_HPP
