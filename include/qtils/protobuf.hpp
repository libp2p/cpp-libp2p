/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/bytes.hpp>

namespace qtils::protobuf {
  template <typename T>
  Bytes encode(const T &pb) {
    Bytes buffer;
    buffer.resize(pb.ByteSizeLong());
    if (not pb.SerializeToArray(buffer.data(), buffer.size())) {
      throw std::logic_error{"qtils::protobuf::encode SerializeToArray"};
    }
    return buffer;
  }
}  // namespace qtils::protobuf
