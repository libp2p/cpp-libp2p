/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/protobuf/protobuf_key.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace testutil {

  using libp2p::crypto::ProtobufKey;
  using libp2p::peer::PeerId;

  PeerId randomPeerId();

}  // namespace testutil
