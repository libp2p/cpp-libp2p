/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/tcp/bytes_counter.hpp>

namespace libp2p::transport {

  ByteCounter &ByteCounter::getInstance() {
    static ByteCounter instance;
    return instance;
  }

  void ByteCounter::incrementBytesRead(uint64_t bytes) {
    bytes_read_.fetch_add(bytes);
  }

  void ByteCounter::incrementBytesWritten(uint64_t bytes) {
    bytes_written_.fetch_add(bytes);
  }

  uint64_t ByteCounter::getBytesRead() const {
    return bytes_read_.load();
  }

  uint64_t ByteCounter::getBytesWritten() const {
    return bytes_written_.load();
  }

}  // namespace libp2p::transport
