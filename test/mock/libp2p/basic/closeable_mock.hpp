/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CLOSEABLE_MOCK_HPP
#define LIBP2P_CLOSEABLE_MOCK_HPP

#include <gmock/gmock.h>
#include "include/libp2p/basic/closeable.hpp"

namespace libp2p::basic {
  class CloseableMock : public Closeable {
   public:
    ~CloseableMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>(void));
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_CLOSEABLE_MOCK_HPP
