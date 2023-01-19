/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_RAWLAYERCONNECTIONMOCK
#define LIBP2P_CONNECTION_RAWLAYERCONNECTIONMOCK

#include <mock/libp2p/connection/raw_connection_mock.hpp>
#include <mock/libp2p/connection/layer_connection_mock.hpp>

#include <gmock/gmock.h>

namespace libp2p::connection {

  class RawConnectionMock : public RawConnection, public LayerConnectionMock {};

}  // namespace libp2p::connection

#endif  // LIBP2P_CONNECTION_RAWLAYERCONNECTIONMOCK
