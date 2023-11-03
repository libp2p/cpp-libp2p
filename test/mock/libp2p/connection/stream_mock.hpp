/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/connection/stream.hpp>

#include <gmock/gmock.h>

namespace libp2p::connection {
  class StreamMock : public Stream {
   public:
    ~StreamMock() override = default;

    StreamMock() = default;
    explicit StreamMock(uint8_t id) : stream_id{id} {}

    /// this field here is for easier testing
    uint8_t stream_id = 137;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD1(close, void(VoidResultHandlerFunc));

    MOCK_METHOD3(read, void(BytesOut, size_t, Reader::ReadCallbackFunc));
    MOCK_METHOD3(readSome, void(BytesOut, size_t, Reader::ReadCallbackFunc));
    MOCK_METHOD3(writeSome, void(BytesIn, size_t, Writer::WriteCallbackFunc));

    MOCK_METHOD2(deferReadCallback,
                 void(outcome::result<size_t>, Reader::ReadCallbackFunc));

    MOCK_METHOD2(deferWriteCallback,
                 void(std::error_code, Writer::WriteCallbackFunc));

    MOCK_METHOD0(reset, void());

    MOCK_CONST_METHOD0(isClosedForRead, bool(void));

    MOCK_CONST_METHOD0(isClosedForWrite, bool(void));

    MOCK_METHOD2(adjustWindowSize, void(uint32_t, VoidResultHandlerFunc));

    MOCK_CONST_METHOD0(isInitiator, outcome::result<bool>());

    MOCK_CONST_METHOD0(remotePeerId, outcome::result<peer::PeerId>());

    MOCK_CONST_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>());

    MOCK_CONST_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>());
  };
}  // namespace libp2p::connection
