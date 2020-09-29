/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_MESSAGE_PATTERNS_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_MESSAGE_PATTERNS_HPP

#include <string>
#include <vector>

namespace libp2p::security::noise {

  enum class MessagePattern : int {  // int required here
    S,
    E,
    DHEE,
    DHES,
    DHSE,
    DHSS,
    PSK  // no elements anymore
  };

  using MessagePatterns = std::vector<std::vector<MessagePattern>>;

  struct HandshakePattern {
    std::string name;
    std::vector<MessagePattern> initiatorPreMessages;
    std::vector<MessagePattern> responderPreMessages;
    MessagePatterns messages;
  };

  HandshakePattern handshakeXX();

}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_CRYPTO_MESSAGE_PATTERNS_HPP
