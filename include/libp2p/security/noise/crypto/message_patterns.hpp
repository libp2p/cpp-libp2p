/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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

  using MP = MessagePattern;

  inline const HandshakePattern handshakeXX{
      .name = "XX",
      .initiatorPreMessages = {},
      .responderPreMessages = {},
      .messages = {
          {MP::E}, {MP::E, MP::DHEE, MP::S, MP::DHES}, {MP::S, MP::DHSE}}};

}  // namespace libp2p::security::noise
