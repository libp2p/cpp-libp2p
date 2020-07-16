/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_MESSAGE_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_MESSAGE_HPP

#include <libp2p/common/types.hpp>
#include <libp2p/crypto/key.hpp>

namespace libp2p::security::noise {

  struct HandshakeMessage {
    crypto::PublicKey identity_key;
    common::ByteArray identity_sig;
    common::ByteArray data;
  };
}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_MESSAGE_HPP
