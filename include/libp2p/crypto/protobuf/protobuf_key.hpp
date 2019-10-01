/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOBUF_KEY_HPP
#define KAGOME_PROTOBUF_KEY_HPP

#include <vector>

namespace libp2p::crypto {
  /**
   * Strict type for key, which is encoded into Protobuf format
   */
  struct ProtobufKey {
    std::vector<uint8_t> key;
  };

  inline bool operator==(const ProtobufKey &f, const ProtobufKey &s) {
    return f.key == s.key;
  }
  inline bool operator!=(const ProtobufKey &f, const ProtobufKey &s) {
    return !(f == s);
  }
}  // namespace libp2p::crypto

#endif  // KAGOME_PROTOBUF_KEY_HPP
