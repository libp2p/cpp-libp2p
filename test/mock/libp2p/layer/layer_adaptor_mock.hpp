/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LAYER_LAYERADAPTORMOCK
#define LIBP2P_LAYER_LAYERADAPTORMOCK

#include <gmock/gmock.h>
#include <libp2p/layer/layer_adaptor.hpp>

namespace libp2p::layer {

  struct LayerAdaptorMock : public LayerAdaptor {
   public:
    ~LayerAdaptorMock() override = default;

    MOCK_METHOD(peer::ProtocolName, getProtocolId, (), (const, override));

    MOCK_METHOD(void, upgradeInbound,
                (std::shared_ptr<connection::LayerConnection>,
                 LayerConnCallbackFunc),
                (const, override));

    MOCK_METHOD(void, upgradeOutbound,
                (std::shared_ptr<connection::LayerConnection>,
                 LayerConnCallbackFunc),
                (const, override));
  };

}  // namespace libp2p::layer

#endif  // LIBP2P_LAYER_LAYERADAPTORMOCK
