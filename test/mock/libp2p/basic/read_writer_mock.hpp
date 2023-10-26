/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/readwriter.hpp>

#include <gmock/gmock.h>

namespace libp2p::basic {
  struct ReadWriterMock : public ReadWriter {
    MOCK_METHOD3(read, void(BytesOut, size_t, Reader::ReadCallbackFunc));
    MOCK_METHOD3(readSome, void(BytesOut, size_t, Reader::ReadCallbackFunc));

    MOCK_METHOD3(write, void(BytesIn, size_t, Writer::WriteCallbackFunc));
    MOCK_METHOD3(writeSome, void(BytesIn, size_t, Writer::WriteCallbackFunc));
  };
}  // namespace libp2p::basic
