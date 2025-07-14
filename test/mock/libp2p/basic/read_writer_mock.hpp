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
    MOCK_METHOD2(readSome, void(BytesOut, Reader::ReadCallbackFunc));

    MOCK_METHOD2(writeSome, void(BytesIn, Writer::WriteCallbackFunc));
  };
}  // namespace libp2p::basic
