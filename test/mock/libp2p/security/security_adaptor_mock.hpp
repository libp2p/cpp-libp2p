/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include <libp2p/security/security_adaptor.hpp>

namespace libp2p::security {
  struct SecurityAdaptorMock : public SecurityAdaptor {
   public:
    ~SecurityAdaptorMock() override = default;

    MOCK_CONST_METHOD0(getProtocolId, peer::ProtocolName());

    MOCK_METHOD2(secureInbound,
                 void(std::shared_ptr<connection::LayerConnection>,
                      SecConnCallbackFunc));

    MOCK_METHOD3(secureOutbound,
                 void(std::shared_ptr<connection::LayerConnection>,
                      const peer::PeerId &,
                      SecConnCallbackFunc));
  };
}  // namespace libp2p::security
