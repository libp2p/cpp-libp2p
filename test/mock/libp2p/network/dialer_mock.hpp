/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_DIALER_MOCK_HPP
#define LIBP2P_DIALER_MOCK_HPP

#include <libp2p/network/dialer.hpp>

#include <gmock/gmock.h>

namespace libp2p::network {

  struct DialerMock : public Dialer {
    ~DialerMock() override = default;

    MOCK_METHOD2(dial, void(const peer::PeerInfo &, DialResultFunc));
    MOCK_METHOD3(dial,
                 void(const peer::PeerInfo &, DialResultFunc,
                      std::chrono::milliseconds));
    MOCK_METHOD3(newStream,
                 void(const peer::PeerInfo &, const peer::Protocol &,
                      StreamResultFunc));
    MOCK_METHOD4(newStream,
                 void(const peer::PeerInfo &, const peer::Protocol &,
                      StreamResultFunc, std::chrono::milliseconds));
    MOCK_METHOD3(newStream,
                 void(const peer::PeerId &, const peer::Protocol &,
                     StreamResultFunc));
  };

}  // namespace libp2p::network

#endif  // LIBP2P_DIALER_MOCK_HPP
