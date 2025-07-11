/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/readwritecloser.hpp>

#include <gmock/gmock.h>

namespace libp2p::basic {
  class ReadWriteCloserMock : public ReadWriteCloser {
   public:
    ~ReadWriteCloserMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>(void));

    MOCK_METHOD2(readSome, void(BytesOut, Reader::ReadCallbackFunc));
    MOCK_METHOD2(writeSome, void(BytesIn, Writer::WriteCallbackFunc));
  };
}  // namespace libp2p::basic
