/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <mock/libp2p/connection/layer_connection_mock.hpp>
#include <mock/libp2p/connection/raw_connection_mock.hpp>

#include <gmock/gmock.h>

namespace libp2p::connection {

  class RawConnectionMock : public RawConnection, public LayerConnectionMock {};

}  // namespace libp2p::connection
