/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_WRITER_MOCK_HPP
#define LIBP2P_WRITER_MOCK_HPP

#include <libp2p/basic/writer.hpp>

#include <gmock/gmock.h>

namespace libp2p::basic {
  class WriterMock : public Writer {
   public:
    ~WriterMock() override = default;

    MOCK_METHOD2(write,
                 void(BytesIn, Writer::WriteCallbackFunc));
    MOCK_METHOD2(writeSome,
                 void(BytesIn, Writer::WriteCallbackFunc));
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_WRITER_MOCK_HPP
