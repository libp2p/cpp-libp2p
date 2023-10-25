/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/types.hpp>
#include <libp2p/crypto/key.hpp>

namespace libp2p::security::noise {

  struct HandshakeMessage {
    crypto::PublicKey identity_key;
    Bytes identity_sig;
    Bytes data;
  };
}  // namespace libp2p::security::noise
