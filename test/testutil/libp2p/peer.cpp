/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <testutil/libp2p/peer.hpp>
#include <testutil/libp2p/random.hpp>

namespace testutil {

  PeerId randomPeerId() {
    auto rand_key = randomBytes(32);
    return PeerId::fromPublicKey(libp2p::crypto::ProtobufKey{rand_key}).value();
  }

}  // namespace testutil
