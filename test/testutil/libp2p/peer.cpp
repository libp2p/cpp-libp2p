/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <testutil/libp2p/peer.hpp>

namespace testutil {

  PeerId randomPeerId() {
    std::vector<uint8_t> rand_key(32, 0);
    for (auto i = 0u; i < 32u; i++) {
      rand_key[i] = (rand() & 0xffu);  // NOLINT
    }
    return PeerId::fromPublicKey(libp2p::crypto::ProtobufKey{rand_key}).value();
  }

}  // namespace testutil
