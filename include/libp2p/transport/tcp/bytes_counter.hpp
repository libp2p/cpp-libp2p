/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>

namespace libp2p::transport {

  class ByteCounter {
   private:
    std::atomic<uint64_t> bytes_read_{0};
    std::atomic<uint64_t> bytes_written_{0};

    ByteCounter() = default;

   public:
    ~ByteCounter() = default;

    void incrementBytesRead(uint64_t bytes);

    void incrementBytesWritten(uint64_t bytes);

    uint64_t getBytesRead() const;

    uint64_t getBytesWritten() const;

    static ByteCounter &getInstance();

    ByteCounter(const ByteCounter &) = delete;
    ByteCounter &operator=(const ByteCounter &) = delete;

    ByteCounter(ByteCounter &&) = delete;
    ByteCounter &operator=(ByteCounter &&) = delete;
  };

}  // namespace libp2p::transport
