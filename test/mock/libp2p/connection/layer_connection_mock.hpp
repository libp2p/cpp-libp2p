/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/connection/layer_connection.hpp>

#include <gmock/gmock.h>

namespace libp2p::connection {

  class LayerConnectionMock : public virtual LayerConnection {
   public:
    ~LayerConnectionMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>());

    MOCK_METHOD3(read, void(BytesOut, size_t, Reader::ReadCallbackFunc));
    MOCK_METHOD3(readSome, void(BytesOut, size_t, Reader::ReadCallbackFunc));
    MOCK_METHOD3(write, void(BytesIn, size_t, Writer::WriteCallbackFunc));
    MOCK_METHOD3(writeSome, void(BytesIn, size_t, Writer::WriteCallbackFunc));
    MOCK_METHOD2(deferReadCallback,
                 void(outcome::result<size_t>, Reader::ReadCallbackFunc));
    MOCK_METHOD2(deferWriteCallback,
                 void(std::error_code, Writer::WriteCallbackFunc));

    bool isInitiator() const noexcept override {
      return isInitiator_hack();
    }

    MOCK_CONST_METHOD0(isInitiator_hack, bool());

    MOCK_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>());

    MOCK_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>());
  };

}  // namespace libp2p::connection
