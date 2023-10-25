/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/transport/upgrader.hpp>

#include <gmock/gmock.h>

#include <libp2p/muxer/yamux.hpp>
#include <libp2p/security/plaintext.hpp>

namespace libp2p::transport {

  class UpgraderMock : public Upgrader {
   public:
    ~UpgraderMock() override = default;
    MOCK_METHOD(void,
                upgradeLayersOutbound,
                (const multi::Multiaddress &,
                 RawSPtr,
                 ProtoAddrVec,
                 OnLayerCallbackFunc),
                (override));

    MOCK_METHOD(void,
                upgradeLayersInbound,
                (RawSPtr, ProtoAddrVec, OnLayerCallbackFunc),
                (override));

    MOCK_METHOD(void,
                upgradeToSecureOutbound,
                (Upgrader::LayerSPtr,
                 const peer::PeerId &,
                 Upgrader::OnSecuredCallbackFunc));

    MOCK_METHOD(void,
                upgradeToSecureInbound,
                (Upgrader::LayerSPtr, Upgrader::OnSecuredCallbackFunc),
                (override));

    MOCK_METHOD(void,
                upgradeToMuxed,
                (Upgrader::SecSPtr, Upgrader::OnMuxedCallbackFunc),
                (override));
  };

}  // namespace libp2p::transport
