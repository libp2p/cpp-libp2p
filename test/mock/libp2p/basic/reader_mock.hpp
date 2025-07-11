/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/reader.hpp>

#include <gmock/gmock.h>

namespace libp2p::basic {
  class ReaderMock : public Reader {
   public:
    ~ReaderMock() override = default;

    MOCK_METHOD2(readSome, void(BytesOut, Reader::ReadCallbackFunc));
  };
}  // namespace libp2p::basic
