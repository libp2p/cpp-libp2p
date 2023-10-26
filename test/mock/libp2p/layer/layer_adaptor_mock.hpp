/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include <libp2p/layer/layer_adaptor.hpp>

namespace libp2p::layer {

  struct LayerAdaptorMock : public LayerAdaptor {
   public:
    ~LayerAdaptorMock() override = default;

    MOCK_METHOD(multi::Protocol::Code, getProtocol, (), (const, override));

    MOCK_METHOD(void,
                upgradeInbound,
                (std::shared_ptr<connection::LayerConnection>,
                 LayerConnCallbackFunc),
                (const, override));

    MOCK_METHOD(void,
                upgradeOutbound,
                (const multi::Multiaddress &,
                 std::shared_ptr<connection::LayerConnection>,
                 LayerConnCallbackFunc),
                (const, override));
  };

}  // namespace libp2p::layer
