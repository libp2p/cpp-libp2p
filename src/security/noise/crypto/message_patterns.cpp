/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/crypto/message_patterns.hpp>

namespace libp2p::security::noise {

  using MP = MessagePattern;

  HandshakePattern handshakeXX() {
    return HandshakePattern{
        .name = "XX",
        .initiatorPreMessages = {},
        .responderPreMessages = {},
        .messages = {
            {MP::E}, {MP::E, MP::DHEE, MP::S, MP::DHES}, {MP::S, MP::DHSE}}};
  }
}  // namespace libp2p::security::noise
