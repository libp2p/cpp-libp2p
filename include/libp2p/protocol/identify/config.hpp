/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/stream_protocols.hpp>

namespace {
  const std::string kIdentifyProto = "/ipfs/id/1.0.0";
}  // namespace

namespace libp2p::protocol {

  struct IdentifyConfig {
    IdentifyConfig() = default;
    StreamProtocols protocols = {::kIdentifyProto};
  };

}  // namespace libp2p::protocol
