/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TESTUTIL_PEER_HPP
#define LIBP2P_TESTUTIL_PEER_HPP

#include "libp2p/crypto/key.hpp"
#include "libp2p/multi/multihash.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace testutil {

  using libp2p::crypto::PublicKey;
  using libp2p::multi::Multihash;
  using libp2p::peer::PeerId;
  using T = libp2p::crypto::PublicKey::Type;

  PeerId randomPeerId();

}  // namespace testutil

#endif  // LIBP2P_TESTUTIL_PEER_HPP
