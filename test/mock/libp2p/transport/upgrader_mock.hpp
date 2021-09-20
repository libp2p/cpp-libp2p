/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_UPGRADER_MOCK_HPP
#define LIBP2P_UPGRADER_MOCK_HPP

#include <libp2p/transport/upgrader.hpp>

#include <gmock/gmock.h>

#include <libp2p/muxer/yamux.hpp>
#include <libp2p/security/plaintext.hpp>

namespace libp2p::transport {

  class UpgraderMock : public Upgrader {
   public:
    ~UpgraderMock() override = default;

    MOCK_METHOD3(upgradeToSecureOutbound,
                 void(Upgrader::RawSPtr, const peer::PeerId &,
                      Upgrader::OnSecuredCallbackFunc));

    MOCK_METHOD2(upgradeToSecureInbound,
                 void(Upgrader::RawSPtr, Upgrader::OnSecuredCallbackFunc));

    MOCK_METHOD2(upgradeToMuxed,
                 void(Upgrader::SecSPtr, Upgrader::OnMuxedCallbackFunc));
  };

}  // namespace libp2p::transport

#endif  // LIBP2P_UPGRADER_MOCK_HPP
