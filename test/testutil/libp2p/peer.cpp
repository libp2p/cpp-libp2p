/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/libp2p/peer.hpp"

namespace testutil {

  PeerId randomPeerId() {
    PublicKey k;

    k.type = T::ED25519;
    k.data.resize(32u);
    for (auto i = 0u; i < 32u; i++) {
      k.data[i] = (rand() & 0xff);
    }

    return PeerId::fromPublicKey(k);
  }

}  // namespace testutil
