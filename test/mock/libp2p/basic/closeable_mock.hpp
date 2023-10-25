/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/closeable.hpp>

#include <gmock/gmock.h>

namespace libp2p::basic {
  class CloseableMock : public Closeable {
   public:
    ~CloseableMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>(void));
  };
}  // namespace libp2p::basic
