/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/network/dialer.hpp>

#include <gmock/gmock.h>

namespace libp2p::network {

  struct DialerMock : public Dialer {
    ~DialerMock() override = default;

    MOCK_METHOD2(dial, void(const peer::PeerInfo &, DialResultFunc));
    MOCK_METHOD3(dial,
                 void(const peer::PeerInfo &,
                      DialResultFunc,
                      std::chrono::milliseconds));
    MOCK_METHOD4(newStream,
                 void(const peer::PeerInfo &,
                      StreamProtocols,
                      StreamAndProtocolOrErrorCb,
                      std::chrono::milliseconds));
    MOCK_METHOD3(newStream,
                 void(const peer::PeerId &,
                      StreamProtocols,
                      StreamAndProtocolOrErrorCb));
  };

}  // namespace libp2p::network
