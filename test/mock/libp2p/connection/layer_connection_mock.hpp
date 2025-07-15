/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/connection/layer_connection.hpp>

#include <gmock/gmock.h>

namespace libp2p::connection {

  class LayerConnectionMock
      : public std::enable_shared_from_this<LayerConnectionMock>,
        public virtual LayerConnection {
   public:
    ~LayerConnectionMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>());

    MOCK_METHOD2(readSome, void(BytesOut, Reader::ReadCallbackFunc));
    MOCK_METHOD2(writeSome, void(BytesIn, Writer::WriteCallbackFunc));
    MOCK_METHOD2(deferReadCallback,
                 void(outcome::result<size_t>, Reader::ReadCallbackFunc));
    MOCK_METHOD2(deferWriteCallback,
                 void(std::error_code, Writer::WriteCallbackFunc));

    bool isInitiator() const override {
      return isInitiator_hack();
    }

    MOCK_CONST_METHOD0(isInitiator_hack, bool());

    MOCK_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>());

    MOCK_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>());
  };

}  // namespace libp2p::connection
