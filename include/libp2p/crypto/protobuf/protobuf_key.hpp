/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

#include <boost/operators.hpp>

namespace libp2p::crypto {
  /**
   * Strict type for key, which is encoded into Protobuf format
   */
  struct ProtobufKey : public boost::equality_comparable<ProtobufKey> {
    explicit ProtobufKey(std::vector<uint8_t> key) : key{std::move(key)} {}

    std::vector<uint8_t> key;

    bool operator==(const ProtobufKey &other) const {
      return key == other.key;
    }
  };
}  // namespace libp2p::crypto
