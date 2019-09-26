/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UPGRADER_MOCK_HPP
#define KAGOME_UPGRADER_MOCK_HPP

#include "p2p/transport/upgrader.hpp"

#include <gmock/gmock.h>
#include "p2p/muxer/yamux.hpp"
#include "p2p/security/plaintext.hpp"

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

#endif  // KAGOME_UPGRADER_MOCK_HPP
